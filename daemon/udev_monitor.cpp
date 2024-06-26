/* File: udev_monitor.cpp

  Copyright (C)   2024
  Author: Oleg Proskurin, <proskurinov@basealt.ru>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <https://www.gnu.org/licenses/>.

*/

#include "udev_monitor.hpp"
#include "custom_mount.hpp"
#include "dal/dto.hpp"
#include "dal/local_storage.hpp"
#include "usb_udev_device.hpp"
#include "utils.hpp"
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <future>
#include <libudev.h>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/select.h>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace usbmount {
// NOLINTNEXTLINE(misc-include-cleaner)
UdevMonitor::UdevMonitor(std::shared_ptr<spdlog::logger> logger)
    : logger_(std::move(logger)), future_obj_(stop_signal_.get_future()),
      udev_(udev_new(), udev_unref),
      monitor_(udev_monitor_new_from_netlink(udev_.get(), "udev"),
               udev_monitor_unref),
      dbase_(dal::LocalStorage::GetStorage()), udef_fd_{0} {
  if (!udev_ || !monitor_) {
    throw std::runtime_error("Can't connect to udev");
  }
  int res = udev_monitor_filter_add_match_subsystem_devtype(monitor_.get(),
                                                            "usb", nullptr);
  if (res < 0) {
    throw std::runtime_error("Error modifiing udev filters");
  }
  res = udev_monitor_filter_add_match_subsystem_devtype(monitor_.get(), "block",
                                                        nullptr);
  if (res < 0) {
    throw std::runtime_error("Error modifiing udev filters");
  }
  res = udev_monitor_enable_receiving(monitor_.get());
  if (res < 0) {
    throw std::runtime_error("Error enabling udev monitor");
  }
  udef_fd_ = udev_monitor_get_fd(monitor_.get());
  std::stringstream str_id;
  str_id << std::this_thread::get_id();
  logger_->debug("Udev monitor constructed in thread {}", str_id.str());
}

void UdevMonitor::Run() noexcept {
  // review local mounts
  ReviewConnectedDevices();
  // Mount devices if not mounted on start
  ApplyMountRulesIfNotMounted();
  uint64_t iteration_counter = 0;
  std::future<void> fut_review_mounts;
  while (!StopRequested()) {
    // wait for new device
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(udef_fd_, &fds);
    // NOLINTNEXTLINE(misc-include-cleaner)
    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000000; // 1sec
    const int ret = select(udef_fd_ + 1, &fds, nullptr, nullptr, &timeout);
    if (ret == -1) {
      logger_->error("select() systemcall returned error");
      logger_->error(utils::SafeErrorNoToStr());
      continue;
    }
    // timeout
    if (ret == 0) {
      // review table every 10min
      ++iteration_counter;
      if (iteration_counter >= 600) {
        iteration_counter = 0;
        fut_review_mounts = std::async(
            std::launch::async, &UdevMonitor::ReviewConnectedDevices, this);
        // std::async(std::launch::async, utils::ReviewMountPoints, logger_);
      }
      continue;
    }
    // new device found
    if (ret > 0 && FD_ISSET(udef_fd_, &fds)) {
      ProcessDevice();
    }
  }
  logger_->info("Stop signal recieved");
}

void UdevMonitor::ProcessDevice() noexcept {
  auto device = RecieveDevice();
  ProcessDevice(std::move(device));
}

void UdevMonitor::ProcessDevice(
    std::shared_ptr<UsbUdevDevice> device) noexcept {
  if (!device) {
    return;
  }
  // there are some rules in db for this device
  const bool device_is_known =
      dbase_->permissions
          .Find(dal::Device({device->vid(), device->pid(), device->serial()}))
          .has_value();
  // the device is added
  const bool device_was_added = device->action() == Action::kAdd;
  // the device is added + known
  const bool known_device_was_added = device_is_known && device_was_added;
  // the device was mounted by this app
  const bool device_was_mounted =
      dbase_->mount_points.Find(device->block_name()).has_value();
  // device is removed + was mounted by this app
  const bool device_removed_and_was_mounted =
      device_was_mounted && device->action() == Action::kRemove;
  const bool fs_is_unsupported = device->filesystem().empty() ||
                                 device->filesystem() == "jfs" ||
                                 device->filesystem() == "LVM2_member";
  if ((known_device_was_added || device_removed_and_was_mounted) &&
      !fs_is_unsupported) {
    utils::MountDevice(std::move(device), logger_);
    return;
  }
  // else - on device change - check the /etc/mtab and compare it with  db
  // maybe local mount table is not valid
  if (device->action() == Action::kChange && device_was_mounted) {
    utils::ReviewMountPoints(logger_);
  }
}

void UdevMonitor::Stop() noexcept { stop_signal_.set_value(); }

bool UdevMonitor::StopRequested() noexcept {
  return !(future_obj_.wait_for(std::chrono::milliseconds(0)) ==
           std::future_status::timeout);
}

std::shared_ptr<UsbUdevDevice> UdevMonitor::RecieveDevice() noexcept {
  std::unique_ptr<udev_device, decltype(&UdevDeviceFree)> device(
      udev_monitor_receive_device(monitor_.get()), UdevDeviceFree);
  if (device) {
    try {
      return std::make_shared<UsbUdevDevice>(std::move(device));
    } catch (const std::exception &ex) {
      // logger_->debug("Constructor of UsbUdevDevice failed");
      // logger_->debug(ex.what());
      return {};
    }
  }
  return {};
}

std::vector<UsbUdevDevice> UdevMonitor::GetConnectedDevices() const noexcept {
  std::vector<UsbUdevDevice> res;
  using utils::udev::UdevEnumerateFree;
  const std::unique_ptr<udev_enumerate, decltype(&UdevEnumerateFree)> enumerate(
      udev_enumerate_new(udev_.get()), UdevEnumerateFree);
  if (!enumerate) {
    logger_->error("[GetConnectedDevices] Can't enumerate devices");
    return res;
  }
  if (udev_enumerate_add_match_subsystem(enumerate.get(), "block") < 0) {
    logger_->error("udev_enumerate_add_match_subsystem error");
    return res;
  }
  udev_enumerate_add_match_property(enumerate.get(), "ID_BUS", "usb");
  udev_enumerate_scan_devices(enumerate.get());
  udev_list_entry *entry = udev_enumerate_get_list_entry(enumerate.get());
  while (entry != nullptr) {
    const char *p_path = udev_list_entry_get_name(entry);
    if (p_path == nullptr) {
      logger_->error("[GetConnectedDevices] udev_list_entry_get_name error ");
      continue;
    }
    std::unique_ptr<udev_device, decltype(&UdevDeviceFree)> device(
        udev_device_new_from_syspath(udev_.get(), p_path), UdevDeviceFree);
    if (!device) {
      logger_->error(
          "[GetConnectedDevices] udev_device_new_from_syspath error ");
      continue;
    }
    try {
      res.emplace_back(std::move(device));
    } catch (const std::exception &ex) {
      logger_->warn("Cant construt a UdevDevice");
      logger_->warn(ex.what());
      continue;
    }
    entry = udev_list_entry_get_next(entry);
  }
  logger_->flush();
  return res;
}

void UdevMonitor::ReviewConnectedDevices() noexcept {
  // first review mountpoints
  utils::ReviewMountPoints(logger_);
  // get a list of connected block devices
  std::unordered_set<std::string> present_devices;
  auto device_objects = GetConnectedDevices();
  for (const auto &dev : device_objects) {
    present_devices.emplace(dev.block_name());
    logger_->debug("[ReviewConnectedDevices] found {}", dev.block_name());
  }
  // get all mounted from db
  auto mountpoints = dbase_->mount_points.GetAll();
  // if a mounted device is not present,unmount it.
  for (const auto &mountpoint : mountpoints) {
    if (present_devices.count(mountpoint.dev_name()) == 0) {
      try {
        auto device = std::make_shared<UsbUdevDevice>(
            DevParams{mountpoint.dev_name(), "remove"});
        CustomMount mounter(device, logger_);
        if (mounter.UnMount()) {
          logger_->info("Unmounted expired {},no such device",
                        device->block_name());
        } else {
          logger_->error("Error unmountiong expired device {}",
                         device->block_name());
        }
      } catch (const std::exception &ex) {
        logger_->error("Can't construnct UsbUdevDeivice for {}",
                       mountpoint.dev_name());
        logger_->error(ex.what());
      }
    }
  }
  logger_->flush();
  // unmount expired devices
}

void UdevMonitor::ApplyMountRulesIfNotMounted() noexcept {
  logger_->debug("[ApplyMountRulesIfNotMounted] Apply rules on start");
  auto device_objects = GetConnectedDevices();
  for (const auto &dev : device_objects) {
    auto device = std::make_shared<UsbUdevDevice>(dev);
    device->SetAction("add");
    logger_->info("[ApplyMountRulesIfNotMounted] found {}",
                  device->block_name());
    auto mountpoints = utils::GetSystemMountedDevices(logger_);
    // if not mounted yet
    if (mountpoints.count(device->block_name()) == 0) {
      logger_->info("process {}", device->block_name());
      ProcessDevice(std::move(device));
    }
  }
}

} // namespace usbmount