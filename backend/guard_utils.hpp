#pragma once

/**
 * @brief Usb-related free-standing functions
 *
 */

#include "guard_rule.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace guard::utils {

///@brief fold list of interface { 03:01:02 03:01:01 } to [03:*:*]
///@param i_type string with list of interfaces from usbguard
std::vector<std::string> FoldUsbInterfacesList(std::string i_type);

/**
 * @brief Creates map vendor ID : vendor Name with in one pass to fil
 * @param vendors set of vendor IDs
 * @return std::map<std::string,std::string> Vendor ID : Vendor Name
 */
std::unordered_map<std::string, std::string>
MapVendorCodesToNames(const std::unordered_set<std::string> &vendors) noexcept;

std::optional<bool>
ExtractDaemonTargetState(boost::json::object *p_obj) noexcept;

std::optional<Target> ExtractTargetPolicy(boost::json::object *p_obj) noexcept;

std::optional<std::string>
ExtractPresetMode(boost::json::object *p_obj) noexcept;

/**
 * @brief Process rules for "manual" mode
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

/**
 * @brief Parses "appended" rules from json
 * @param[in] ptr_json_array_rules A pointer to json array with rules
 * @param rules_to_add Vector, where new rulles will be appended
 * @return JSON object, containig arrays of html ids - "rules_OK" and
 * "rules_BAD"
 * @details This function is called from ProcessJsonManualMode
 */
boost::json::object
ProcessJsonAppended(const boost::json::array *ptr_json_array_rules,
                    std::vector<GuardRule> &rules_to_add) noexcept;

/// @brief Add a rule to allow all HID devices.
/// @param rules_to_add Vector,where a new rule will be appended.
void AddAllowHid(std::vector<GuardRule> &rules_to_add) noexcept;

///@brief Add a rule to block 08 and 06 - usb and mtp.
/// @param rules_to_add Vector,where a new rule will be appended.
void AddBlockUsbStorages(std::vector<GuardRule> &rules_to_add) noexcept;

/// @brief Add rules to reject known android devices
/// @param rules_to_add Vector,where a new rule will be appended.
bool AddRejectAndroid(std::vector<GuardRule> &rules_to_add) noexcept;

} // namespace guard::utils