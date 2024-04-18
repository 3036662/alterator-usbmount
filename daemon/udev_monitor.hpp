#pragma once
#include "dal/local_storage.hpp"
#include "usb_udev_device.hpp"
#include <future>
#include <libudev.h>
#include <memory.h>
#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <vector>

namespace usbmount {

class UdevMonitor {
public:
  explicit UdevMonitor(std::shared_ptr<spdlog::logger> &logger);

  void Run() noexcept;
  void Stop() noexcept;
  std::vector<UsbUdevDevice> GetConnectedDevices() const noexcept;

private:
  bool StopRequested() noexcept;
  void ProcessDevice() noexcept;

  void ReviewConnectedDevices() noexcept;
  std::shared_ptr<UsbUdevDevice> RecieveDevice() noexcept;

  std::shared_ptr<spdlog::logger> &logger_;
  std::promise<void> stop_signal_;
  std::future<void> future_obj_;
  std::unique_ptr<udev, decltype(&udev_unref)> udev_;
  std::unique_ptr<udev_monitor, decltype(&udev_monitor_unref)> monitor_;
  std::shared_ptr<dal::LocalStorage> dbase_;
  int udef_fd_;
};

} // namespace usbmount