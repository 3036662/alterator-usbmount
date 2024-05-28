/* File: udev_monitor.hpp

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

#pragma once
#include "dal/local_storage.hpp"
#include "usb_udev_device.hpp"
#include <future>
#include <libudev.h>
#include <memory>
#include <spdlog/logger.h>
#include <vector>

namespace usbmount {

class UdevMonitor {
public:
  explicit UdevMonitor(std::shared_ptr<spdlog::logger> logger);

  /// @brief start device monitor
  void Run() noexcept;
  /// @brief stop monitor thread
  void Stop() noexcept;
  std::vector<UsbUdevDevice> GetConnectedDevices() const noexcept;

private:
  bool StopRequested() noexcept;
  void ProcessDevice() noexcept;
  void ProcessDevice(std::shared_ptr<UsbUdevDevice> device) noexcept;

  /**
   * @brief Review connected devices and unmount mountpoints for which devices
   * are not present
   *
   */
  void ReviewConnectedDevices() noexcept;

  /**
   * @brief Mount present device is they are not mounted
   */
  void ApplyMountRulesIfNotMounted() noexcept;

  /**
   * @brief Recieve a device from udev
   * @return std::shared_ptr<UsbUdevDevice>
   */
  std::shared_ptr<UsbUdevDevice> RecieveDevice() noexcept;

  std::shared_ptr<spdlog::logger> logger_;
  std::promise<void> stop_signal_;
  std::future<void> future_obj_;
  std::unique_ptr<udev, decltype(&udev_unref)> udev_;
  std::unique_ptr<udev_monitor, decltype(&udev_monitor_unref)> monitor_;
  std::shared_ptr<dal::LocalStorage> dbase_;
  int udef_fd_;
};

} // namespace usbmount