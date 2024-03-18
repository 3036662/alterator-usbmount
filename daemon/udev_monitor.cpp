#include "udev_monitor.hpp"
#include "usb_udev_device.hpp"
#include <iostream>
#include <libudev.h>
#include <memory>
#include <optional>
#include <stdexcept>

UdevMonitor::UdevMonitor()
    : udev_(udev_new(), udev_unref),
      monitor_(udev_monitor_new_from_netlink(udev_.get(), "udev"),
               udev_monitor_unref) {
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
  // monitor_fd_=udev_monitor_get_fd(monitor_.get());
  res = udev_monitor_enable_receiving(monitor_.get());
  if (res < 0)
    throw std::runtime_error("Error enabling udev monitor");
}

std::optional<UsbUdevDevice> UdevMonitor::RecieveDevice() {
  std::unique_ptr<udev_device, decltype(&udev_device_unref)> device(
      udev_monitor_receive_device(monitor_.get()), udev_device_unref);
  if (device) {
    const char *p_action =
        udev_device_get_property_value(device.get(), "ACTION");
    std::string action;
    if (p_action != NULL)
      action = p_action;
    const char *p_vid =
        udev_device_get_property_value(device.get(), "ID_VENDOR_ID");
    std::string vid;
    if (p_vid != NULL)
      vid = p_vid;
    const char *p_pid =
        udev_device_get_property_value(device.get(), "ID_MODEL_ID");
    std::string pid;
    if (p_pid != NULL)
      pid = p_pid;
    const char *p_subsystem =
        udev_device_get_property_value(device.get(), "SUBSYSTEM");
    std::string subsystem;
    if (p_subsystem != NULL)
      subsystem = p_subsystem;

    if (subsystem == "block" && action == "add") {
      const char *p_block =
          udev_device_get_property_value(device.get(), "DEVNAME");
      std::string block;
      if (p_block != NULL) {
        block = p_block;
        return UsbUdevDevice{std::move(action), std::move(vid), std::move(pid),
                             std::move(subsystem), std::move(block)};
      }
    }
  }
  return std::nullopt;
}