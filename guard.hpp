#pragma once
#include "usb_device.hpp"
#include <IPCClient.hpp>
#include <USBGuard.hpp>
#include <memory>

class Guard {
public:
  // guard can be constructed even if daemon in not active
  Guard();

  std::vector<UsbDevice> ListCurrentUsbDevices();

  bool AllowOrBlockDevice(std::string id, bool allow = false,
                          bool permanent = true);

private:
  const std::string default_query = "match";
  std::unique_ptr<usbguard::IPCClient> ptr_ipc;

  // check if daemon is active
  bool HealthStatus() const;
};