#pragma once
#include "usb_udev_device.hpp"
#include <libudev.h>
#include <memory.h>
#include <memory>
#include <optional>

class UdevMonitor {
public:
  UdevMonitor();
  std::optional<UsbUdevDevice> RecieveDevice();

private:
  std::unique_ptr<udev, decltype(&udev_unref)> udev_;
  std::unique_ptr<udev_monitor, decltype(&udev_monitor_unref)> monitor_;
};