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

  std::optional<std::vector<guard::GuardRule>>
  DeleteRules(const std::vector<uint> &rule_indexes) noexcept;
  std::optional<std::string>
  ParseJsonRulesChanges(const std::string &msg) noexcept;

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

  /**
   * @brief Process rules for "manual" mode
   *
   * @param ptr_jobj Json object, containig "appended_rules" and "deleted_rules"
   * arrays
   * @param[out] rules_to_delete array,where to put order numbers of rules to
   * delete
   * @param[out] rules_to_add array, where to put rules to append
   * @return boost::json::object, containig "rules_OK" and "rules_BAD" arrays
   * @details rules_OK and rules_BAD contains html <tr> ids for validation
   */
  boost::json::object
  ProcessJsonManualMode(const boost::json::object *ptr_jobj,
                        std::vector<uint> &rules_to_delete,
                        std::vector<GuardRule> &rules_to_add) noexcept;

  boost::json::object
  ProcessJsonAllowConnected(std::vector<GuardRule> &rules_to_add) noexcept;

  /**
   * @brief Add a rule to allow all HID devices.
   * 
   * @param rules_to_add Vector,where a new rule will be appended.
   */
  void AddAllowHid(std::vector<GuardRule> &rules_to_add) noexcept;

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

} // namespace guard