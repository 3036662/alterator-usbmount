#pragma once
#include "config_status.hpp"
#include "usb_device.hpp"
#include <IPCClient.hpp>
#include <USBGuard.hpp>
#include <memory>
#include <string>
#include <unordered_set>

namespace guard {

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
  /**
   * @brief check configuration of UsbGuard daemon
   * @return ConfigStatus object
   */
  ConfigStatus GetConfigStatus();
  /**
   * @brief Creates map vendor ID : vendor Name with in one pass to file
   *
   * @param vendors set of vendor IDs
   * @return std::map<std::string,std::string> Vendor ID : Vendor Name
   */
  std::unordered_map<std::string, std::string>
  MapVendorCodesToNames(const std::unordered_set<std::string> vendors) const;

  bool DeleteRules(const std::vector<uint> &rule_indexes);
  std::string ParseJsonRulesChanges(const std::string &msg) noexcept;

private:
  const std::string default_query = "match";
  std::unique_ptr<usbguard::IPCClient> ptr_ipc;
  /// True if daemon is active
  bool HealthStatus() const;
  /// try to connect the UsbGuardDaemon
  void ConnectToUsbGuard() noexcept;

  ///@brief fold list of interface { 03:01:02 03:01:01 } to [03:*:*]
  ///@param i_type string with list of interfaces from usbguard
  std::vector<std::string> FoldUsbInterfacesList(std::string i_type) const;

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

} // namespace guard