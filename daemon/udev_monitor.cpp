#include "udev_monitor.hpp"
#include "dal/dto.hpp"
#include "dal/local_storage.hpp"
#include "usb_udev_device.hpp"
#include "utils.hpp"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <exception>
#include <future>
#include <iostream>
#include <libudev.h>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace usbmount {

UdevMonitor::UdevMonitor(std::shared_ptr<spdlog::logger> &logger)
    : logger_(logger), future_obj_(stop_signal_.get_future()),
      udev_(udev_new(), udev_unref),
      monitor_(udev_monitor_new_from_netlink(udev_.get(), "udev"),
               udev_monitor_unref),
      dbase_(dal::LocalStorage::GetStorage()), udef_fd_{0} {
  if (!udev_ || !monitor_)
    throw std::runtime_error("Can't connect to udev");
  int res = udev_monitor_filter_add_match_subsystem_devtype(monitor_.get(),
                                                            "usb", NULL);
  if (res < 0)
    throw std::runtime_error("Error modifiing udev filters");
  res = udev_monitor_filter_add_match_subsystem_devtype(monitor_.get(), "block",
                                                        NULL);
  if (res < 0)
    throw std::runtime_error("Error modifiing udev filters");
  res = udev_monitor_enable_receiving(monitor_.get());
  if (res < 0)
    throw std::runtime_error("Error enabling udev monitor");
  udef_fd_ = udev_monitor_get_fd(monitor_.get());
  std::stringstream str_id;
  str_id << std::this_thread::get_id();
  logger_->debug("Udev monitor constructed in thread {}", str_id.str());
}

void UdevMonitor::Run() noexcept {
  std::stringstream str_id;
  str_id << std::this_thread::get_id();
  logger_->debug("Udev monitor is running in thread {}", str_id.str());
  std::list<std::future<void>> futures;
  while (!StopRequested()) {
    // wait for new device
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(udef_fd_, &fds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    int ret = select(udef_fd_ + 1, &fds, NULL, NULL, &timeout);
    if (ret == -1) {
      logger_->error("select() systemcall returned error");
      logger_->error(strerror(errno));
      continue;
    }
    // timeout
    if (ret == 0)
      continue;
    // new device found
    if (ret > 0 && FD_ISSET(udef_fd_, &fds)) {
      ProcessDevice();
    }
  }
  logger_->debug("Stop signal recieved");
}

void UdevMonitor::ProcessDevice() noexcept {
  auto device = RecieveDevice();
  if (!device)
    return;
  bool device_known =
      dbase_->permissions
          .Find(dal::Device({device->vid(), device->pid(), device->serial()}))
          .has_value();
  if (((device->action() == Action::kAdd && device_known) ||
       device->action() == Action::kRemove) &&
      !device->filesystem().empty() && device->filesystem() != "jfs" &&
      device->filesystem() != "LVM2_member") {
    auto begin = std::chrono::high_resolution_clock::now();
    // logger_->debug(device->block_name());
    std::string filesystem = device->filesystem();
    utils::MountDevice(std::move(device), logger_);
    std::cout << "Mount device time = " << filesystem << " "
              << utils::since(begin).count() << "[ms]\n";
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
      logger_->debug("Constructor of UsbUdevDevice failed");
      logger_->debug(ex.what());
      return std::shared_ptr<UsbUdevDevice>();
    }
  }
  return std::shared_ptr<UsbUdevDevice>();
}

} // namespace usbmount