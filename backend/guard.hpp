#pragma once
#include "config_status.hpp"
#include "guard_rule.hpp"
#include "usb_device.hpp"
#include <IPCClient.hpp>
#include <USBGuard.hpp>
#include <memory>
#include <string>

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
  std::vector<UsbDevice> ListCurrentUsbDevices() noexcept;
  /**
   * @brief Allow or block device
   * @param[in] id  string, containing numerical id of usb device
   * @param[in] allow false - block, true - allow
   * @param[in] permanent  true(default) - create permanent UsbGuard rule
   */
  bool AllowOrBlockDevice(const std::string &device_id, bool allow = false,
                          bool permanent = true) noexcept;
  /**
   * @brief check configuration of UsbGuard daemon
   * @return ConfigStatus object
   */
  ConfigStatus GetConfigStatus() noexcept;

  /**
   * @brief Parse a JSON string containing changes, recieved from alterator-usb
   * web-interface
   *
   * @param msg A JSON string containing changes for alterator-usb
   * @return json object, constaining  "STATUS","rules_OK","rules_BAD"
   * @code param example
   *
   * {
   *   "preset_mode": "manual_mode",
   *   "deleted_rules": [
   *     "6",
   *     "7"
   *   ],
   *   "appended_rules": [
   *     {
   *       "table_id": "list_unsorted_rules",
   *       "tr_id": "rule_1",
   *       "target": "allow",
   *       "fields_arr": [
   *         {
   *           "raw_rule": "allow if true"
   *         }
   *       ]
   *     }
   *   ],
   *   "run_daemon": "true",
   *   "policy_type": "radio_white_list"
   * }
   * @endcode
   * @code return example
   *
   * {
   *  "STATUS": "OK",
   *  "rules_BAD": [
   *    "rule_1"
   *   ],
   *  "rules_OK": []
   * }
   * @endcode
   */
  std::optional<std::string>
  ParseJsonRulesChanges(const std::string &msg) noexcept;

private:
  const std::string default_query = "match";
  std::unique_ptr<usbguard::IPCClient> ptr_ipc;
  /// True if daemon is active
  bool HealthStatus() const noexcept;
  /// try to connect the UsbGuardDaemon
  void ConnectToUsbGuard() noexcept;

  /**
   * @brief Reads rules from USB Guard config,
   * deletes by index, and skips rules, conflicting with a new policy
   *
   * @param rule_indexes Order numbers of rules to delete
   * @param new_policy New implicit policy (allow || block)
   * @param[out] new_rules Vector, where the result will be appended
   * @param[out] deleted_by_policy_change If function will skip some
   * rules via conflict with new policy, this flag will be set to true
   * @return true on success
   */
  bool DeleteRules(const std::vector<uint> &rule_indexes, Target new_policy,
                   std::vector<GuardRule> &new_rules,
                   bool &deleted_by_policy_change) noexcept;

  boost::json::object
  ProcessJsonAllowConnected(std::vector<GuardRule> &rules_to_add) noexcept;
  static std::optional<bool>
  ExtractDaemonTargetState(boost::json::object *p_obj) noexcept;

  static std::optional<Target>
  ExtractTargetPolicy(boost::json::object *p_obj) noexcept;

  static std::optional<std::string>
  ExtractPresetMode(boost::json::object *p_obj) noexcept;

  /**
   * @brief Process rules for "manual" mode
   *
   * @param ptr_jobj Json object, containig "appended_rules" and "deleted_rules"
   * arrays
   *
   * @param[out] rules_to_delete array,where to put order numbers of rules to
   * delete
   *
   * @param[out] rules_to_add array, where to put rules to append
   * @return boost::json::object, containig "rules_OK" and "rules_BAD" arrays
   * @details rules_OK and rules_BAD contains html <tr> ids for validation
   */
  static boost::json::object
  ProcessJsonManualMode(const boost::json::object *ptr_jobj,
                        std::vector<uint> &rules_to_delete,
                        std::vector<GuardRule> &rules_to_add) noexcept;

  /**
   * @brief Parses "appended" rules from json
   * @param[in] ptr_json_array_rules A pointer to json array with rules
   * @param rules_to_add Vector, where new rulles will be appended
   * @return JSON object, containig arrays of html ids - "rules_OK" and
   * "rules_BAD"
   * @details This function is called from ProcessJsonManualMode
   */
  static boost::json::object
  ProcessJsonAppended(const boost::json::array *ptr_json_array_rules,
                      std::vector<GuardRule> &rules_to_add) noexcept;
  /**
   * @brief Add a rule to allow all HID devices.
   *
   * @param rules_to_add Vector,where a new rule will be appended.
   */
  static void AddAllowHid(std::vector<GuardRule> &rules_to_add) noexcept;
  /**
   * @brief Add a rule to block 08 and 06 - usb and mtp.
   *
   * @param rules_to_add Vector,where a new rule will be appended.
   */
  static void
  AddBlockUsbStorages(std::vector<GuardRule> &rules_to_add) noexcept;

  /**
   * @brief Add rules to reject known android devices
   *
   * @param rules_to_add Vector,where a new rule will be appended.
   */
  static bool AddRejectAndroid(std::vector<GuardRule> &rules_to_add) noexcept;

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

} // namespace guard