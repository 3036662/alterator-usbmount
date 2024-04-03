#pragma once
#include "usb_udev_device.hpp"
#include <future>
#include <libudev.h>
#include <memory.h>
#include <memory>
#include <optional>
#include <spdlog/logger.h>

class UdevMonitor {
public:
  UdevMonitor(std::shared_ptr<spdlog::logger> &logger);

  void Run() noexcept;
  void Stop() noexcept;

private:
  bool StopRequested() noexcept;
  void ProcessDevice() noexcept;

  std::shared_ptr<UsbUdevDevice> RecieveDevice() noexcept;
  std::shared_ptr<spdlog::logger> &logger_;
  std::promise<void> stop_signal_;
  std::future<void> future_obj_;
  std::unique_ptr<udev, decltype(&udev_unref)> udev_;
  std::unique_ptr<udev_monitor, decltype(&udev_monitor_unref)> monitor_;
  int udef_fd_;
};