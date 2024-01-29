#pragma once
#include "usb_device.hpp"
#include <IPCClient.hpp>
#include <USBGuard.hpp>
#include <memory>

/**
 * @class Guard
 * @brief Wrapper class for USBGuard
 * @warning Guard can be constructed even if daemon in not active
 */
class Guard {
public:
  /// @brief Constructor
  Guard();

  /**
   * @brief List current usb devices
   * @return vector<UsbDevices>
   */
  std::vector<UsbDevice> ListCurrentUsbDevices();
  /**
   * @brief Allow or block device
   * @param[in] id  string, containing numerical id of usb device
   * @param[in] allow false - block, true - allow
   * @param[in] permanent  true(default) - create permanent UsbGuard rule
   */
  bool AllowOrBlockDevice(std::string id, bool allow = false,
                          bool permanent = true);

private:
  const std::string default_query = "match";
  std::unique_ptr<usbguard::IPCClient> ptr_ipc;

  /// True if daemon is active
  bool HealthStatus() const;
};