#include "guard.hpp"
#include "config_status.hpp"
#include "guard_rule.hpp"
#include "guard_utils.hpp"
#include "json_rule.hpp"
#include "log.hpp"
#include "usb_device.hpp"
#include "utils.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_set>

namespace guard {

using namespace guard::utils;
using ::utils::QuoteIfNotQuoted;
using ::utils::StrToUint;
using ::utils::UnQuote;

Guard::Guard() : ptr_ipc(nullptr) { ConnectToUsbGuard(); }

bool Guard::HealthStatus() const noexcept {
  return ptr_ipc && ptr_ipc->isConnected();
}

std::vector<UsbDevice> Guard::ListCurrentUsbDevices() noexcept {
  std::vector<UsbDevice> res;
  if (!HealthStatus())
    return res;
  // health is ok -> get all devices
  std::vector<usbguard::Rule> rules;
  try {
    rules = ptr_ipc->listDevices(default_query);
    for (const usbguard::Rule &rule : rules) {
      std::vector<std::string> i_types =
          FoldUsbInterfacesList(rule.attributeWithInterface().toRuleString());
      for (const std::string &i_type : i_types) {
        std::vector<std::string> vid_pid;
        boost::split(vid_pid, rule.getDeviceID().toString(),
                     [](const char symbol) { return symbol == ':'; });
        UsbDevice::DeviceData dev_data{
            rule.getRuleID(),
            usbguard::Rule::targetToString(rule.getTarget()),
            rule.getName(),
            !vid_pid.empty() ? vid_pid[0] : "",
            vid_pid.size() > 1 ? vid_pid[1] : "",
            rule.getViaPort(),
            rule.getWithConnectType(),
            i_type,
            rule.getSerial(),
            rule.getHash()

        };
        res.emplace_back(std::move(dev_data));
      }
    }
  } catch (const std::exception &ex) {
    Log::Error() << "USBGuard error";
    Log::Error() << ex.what();
    return res;
  }
  // fill all vendor names with one read of file
  std::unordered_set<std::string> vendors_to_search;
  for (const UsbDevice &usb : res) {
    vendors_to_search.insert(usb.vid());
  }
  std::unordered_map<std::string, std::string> vendors_names{
      MapVendorCodesToNames(vendors_to_search)};
  for (UsbDevice &usb : res) {
    if (vendors_names.count(usb.vid()) != 0) {
      usb.vendor_name(vendors_names[usb.vid()]);
    }
  }
  return res;
}

bool Guard::AllowOrBlockDevice(const std::string &device_id, bool allow,
                               bool permanent) noexcept {
  if (device_id.empty() || !HealthStatus())
    return false;
  std::optional<uint32_t> id_numeric = StrToUint(device_id);
  if (!id_numeric)
    return false;
  usbguard::Rule::Target policy =
      allow ? usbguard::Rule::Target::Allow : usbguard::Rule::Target::Block;
  try {
    ptr_ipc->applyDevicePolicy(*id_numeric, policy, permanent);
  } catch (const usbguard::Exception &ex) {
    Log::Error() << "Can't add rule."
                 << "May be rule conflict happened.";
    Log::Error() << ex.what();
    return false;
  }
  return true;
}

ConfigStatus Guard::GetConfigStatus() noexcept {
  ConfigStatus config_status;
  //  TODO if daemon is on active think about creating policy before enabling
  if (!HealthStatus())
    ConnectToUsbGuard();
  config_status.guard_daemon_active = HealthStatus();
  return config_status;
}

bool Guard::DeleteRules(const std::vector<uint> &rule_indexes,
                        const Target new_policy,
                        std::vector<GuardRule> &new_rules,
                        bool &deleted_by_policy_change) noexcept {
  ConfigStatus config = GetConfigStatus();
  // make sure that old rules were successfully parsed
  std::pair<std::vector<guard::GuardRule>, uint> parsed_rules =
      config.ParseGuardRulesFile();
  if (parsed_rules.first.size() != parsed_rules.second) {
    Log::Error() << "The rules file is not completelly parsed, can't edit";
    return false;
  }
  // copy old rules,except listed in rule_indexes
  std::set<uint> unique_indexes(rule_indexes.cbegin(), rule_indexes.cend());
  for (const auto &rule : parsed_rules.first) {
    if (unique_indexes.count(rule.number) == 0) {
      // skip rules with conflicting policy,place rest to new_rules
      // if implicit policy="allow" rules must be block or reject
      if ((new_policy == Target::allow &&
           (rule.target == Target::block || rule.target == Target::reject)) ||
          (new_policy == Target::block && rule.target == Target::allow)) {
        new_rules.push_back(rule);
      } else {
        deleted_by_policy_change = true;
      }
    }
  }
  return true;
}

void Guard::ConnectToUsbGuard() noexcept {
  try {
    ptr_ipc = std::make_unique<usbguard::IPCClient>(true);
  } catch (usbguard::Exception &e) {
    Log::Warning() << "Error connecting to USBGuard daemon.";
    Log::Warning() << e.what();
  }
}

std::optional<std::string>
Guard::ParseJsonRulesChanges(const std::string &msg) noexcept {
  namespace json = boost::json;
  // parse the json and process
  json::value json_value;
  json::object *ptr_jobj = nullptr;
  try {
    json_value = json::parse(msg);
    ptr_jobj = json_value.if_object();
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse JSON";
    Log::Error() << ex.what();
  }
  // daemon target state (active or stopped)
  std::optional<bool> daemon_activate = ExtractDaemonTargetState(ptr_jobj);
  if (!daemon_activate.has_value())
    return std::nullopt;
  // policy type
  std::optional<Target> policy = ExtractTargetPolicy(ptr_jobj);
  if (!policy.has_value())
    return std::nullopt;
  // preset mode
  std::optional<std::string> preset_mode = ExtractPresetMode(ptr_jobj);
  if (!preset_mode.has_value())
    return std::nullopt;
  // new rules will be stored here
  bool rules_changed_by_policy{false};
  std::vector<guard::GuardRule> new_rules;
  std::vector<uint> rules_to_delete;
  std::vector<GuardRule> rules_to_add;
  ConfigStatus config_status = GetConfigStatus();
  // result json
  boost::json::object obj_result;
  // manual mode
  if (*preset_mode == "manual_mode") {
    obj_result = ProcessJsonManualMode(ptr_jobj, rules_to_delete, rules_to_add);
    // if some rules are bad - just return a validation result
    if (!obj_result.at("rules_BAD").as_array().empty()) {
      return json::serialize(obj_result);
    }
    // build a vector with new rules
    if (!DeleteRules(rules_to_delete, *policy, new_rules,
                     rules_changed_by_policy))
      return std::nullopt;
    // change policy (bool) - true means block
    config_status.ChangeImplicitPolicy(*policy == Target::block);
  }
  // block all except connected
  if (*preset_mode == "put_connected_to_white_list" ||
      *preset_mode == "put_connected_to_white_list_plus_HID") {
    obj_result = ProcessJsonAllowConnected(rules_to_add);
  }
  // add HID devices to white list
  if (*preset_mode == "put_connected_to_white_list_plus_HID")
    AddAllowHid(rules_to_add);
  // block only usb mass storage  and MTP PTP
  if (*preset_mode == "put_disks_and_mtp_to_black") {
    AddBlockUsbStorages(rules_to_add);
    config_status.ChangeImplicitPolicy(false);
    obj_result["STATUS"] = "OK";
  }
  // block known android devices
  if (*preset_mode == "block_and_reject_android") {
    if (!AddRejectAndroid(rules_to_add))
      return std::nullopt;
    config_status.ChangeImplicitPolicy(false);
    obj_result["STATUS"] = "OK";
  }
  // add rules_to_add to new rules vector
  for (auto &rule : rules_to_add)
    new_rules.push_back(std::move(rule));
  // build one string for file overwrite
  std::string str_new_rules;
  for (const auto &rule : new_rules) {
    str_new_rules += rule.BuildString(true, true);
    str_new_rules += "\n";
  }
  // overwrite the rules and test launch if some rules were added or deleted
  if (rules_changed_by_policy || !rules_to_delete.empty() ||
      !rules_to_add.empty()) {
    if (!config_status.OverwriteRulesFile(str_new_rules, *daemon_activate)) {
      Log::Error() << "Can't launch the daemon with new rules";
      return std::nullopt;
    }
  }
  if (!config_status.ChangeDaemonStatus(*daemon_activate, *daemon_activate))
    Log::Error() << "Change the daemon status FAILED";
  return json::serialize(obj_result);
}

boost::json::object
Guard::ProcessJsonManualMode(const boost::json::object *ptr_jobj,
                             std::vector<uint> &rules_to_delete,
                             std::vector<GuardRule> &rules_to_add) noexcept {
  namespace json = boost::json;
  // if some new rules were added
  // put new rules to vector rules_to_add
  // put rules ids to obj_result["rules_OK"] and obj_result["rules_BAD"]
  json::object obj_result;
  obj_result["rules_OK"] = json::array();
  obj_result["rules_BAD"] = json::array();
  if (ptr_jobj != nullptr && ptr_jobj->contains("appended_rules")) {
    const json::array *ptr_json_array_rules =
        ptr_jobj->at("appended_rules").if_array();
    if (ptr_json_array_rules != nullptr && !ptr_json_array_rules->empty()) {
      obj_result = ProcessJsonAppended(ptr_json_array_rules, rules_to_add);
    }
  }
  // if we need to remove some rules
  // put rules numbers to "rules_to_delete" vector
  if (ptr_jobj->contains("deleted_rules") &&
      ptr_jobj->at("deleted_rules").is_array()) {
    for (const auto &element : ptr_jobj->at("deleted_rules").as_array()) {
      if (!element.is_string())
        continue;
      auto id_rule = StrToUint(element.as_string().c_str());
      if (id_rule.has_value()) {
        rules_to_delete.push_back(*id_rule);
      }
    }
  }
  // put list of validated string to response json
  if (obj_result.contains("rules_BAD")) {
    obj_result["STATUS"] =
        obj_result.at("rules_BAD").as_array().empty() ? "OK" : "BAD";
  } else {
    obj_result["STATUS"] = "BAD";
  }
  return obj_result;
}

boost::json::object
Guard::ProcessJsonAppended(const boost::json::array *ptr_json_array_rules,
                           std::vector<GuardRule> &rules_to_add) noexcept {
  using guard::utils::json::JsonRule;
  boost::json::array json_arr_OK;
  boost::json::array json_arr_BAD;
  for (const auto &rule : *ptr_json_array_rules) {
    const boost::json::object *ptr_json_rule = rule.if_object();
    if (ptr_json_rule != nullptr && ptr_json_rule->contains("tr_id")) {
      const boost::json::string *tr_id = ptr_json_rule->at("tr_id").if_string();
      // try to build a rule
      try {
        JsonRule json_rule(ptr_json_rule);
        GuardRule rule{json_rule.BuildString()};
        rules_to_add.push_back(std::move(rule));
        if (tr_id != nullptr && !tr_id->empty()) {
          json_arr_OK.emplace_back(*tr_id);
        }
      }
      // if failed
      catch (const std::logic_error &ex) {
        Log::Error() << "Can't build the rule";
        Log::Error() << ex.what();
        if (tr_id != nullptr && !tr_id->empty()) {
          json_arr_BAD.emplace_back(*tr_id);
        }
      }
    }
  }
  boost::json::object obj_result;
  obj_result["rules_OK"] = std::move(json_arr_OK);
  obj_result["rules_BAD"] = std::move(json_arr_BAD);
  return obj_result;
}

boost::json::object Guard::ProcessJsonAllowConnected(
    std::vector<GuardRule> &rules_to_add) noexcept {
  namespace json = boost::json;
  json::object res;
  auto config = GetConfigStatus();
  // temporary change the policy to "allow all"
  // The purpose is to launch USBGuard without blocking anything
  // to receive a list of devices
  if (!config.ChangeImplicitPolicy(false)) {
    Log::Error() << "Can't change usbguard policy";
    res["STATUS"] = "error";
    res["ERROR_MSG"] = "Failed to change usbguard policy";
    return res;
  }
  Log::Info() << "USBguard implicit policy was changed to allow all";
  // make sure that USBGuard is running
  ConnectToUsbGuard();
  if (!HealthStatus()) {
    config.TryToRun(true);
    ConnectToUsbGuard();
    if (!HealthStatus()) {
      Log::Error() << "Can't launch usbguard";
      res["STATUS"] = "error";
      res["ERROR_MSG"] = "Failed to create policy.Can't launch usbguard";
      return res;
    }
  }
  // get list of devices
  std::vector<UsbDevice> devs = ListCurrentUsbDevices();
  for (const UsbDevice &dev : devs) {
    std::stringstream string_builder;
    string_builder << "allow name " << QuoteIfNotQuoted(dev.name()) << " hash "
                   << QuoteIfNotQuoted(dev.hash());
    try {
      rules_to_add.emplace_back(string_builder.str());
    } catch (const std::logic_error &ex) {
      Log::Error() << "Can't create a rule for device"
                   << "allow name " << QuoteIfNotQuoted(dev.name()) << " hash "
                   << QuoteIfNotQuoted(dev.hash());
      Log::Error() << ex.what();
      res["STATUS"] = "error";
      res["ERROR_MSG"] = "Failed to create policy.Failed";
      return res;
    }
  }

  if (config.implicit_policy_target != "block") {
    Log::Info() << "Changing USBGuard policy to block all";
    config.ChangeImplicitPolicy(true);
  }
  res["STATUS"] = "OK";
  return res;
}

void Guard::AddAllowHid(std::vector<GuardRule> &rules_to_add) noexcept {
  try {
    rules_to_add.emplace_back("allow with-interface 03:*:*");
  } catch (const std::logic_error &ex) {
    Log::Error() << "Can't add a rules for HID devices";
    Log::Error() << ex.what();
  }
}

void Guard::AddBlockUsbStorages(std::vector<GuardRule> &rules_to_add) noexcept {
  try {
    rules_to_add.emplace_back("block with-interface 08:*:*");
    rules_to_add.emplace_back("block with-interface 06:*:*");
  } catch (const std::logic_error &ex) {
    Log::Error() << "Can't add a rules for USB storages";
    Log::Error() << ex.what();
  }
}

bool Guard::AddRejectAndroid(std::vector<GuardRule> &rules_to_add) noexcept {
  const std::string path_to_vidpid = "/etc/usbguard/android_vidpid.json";
  try {
    if (!std::filesystem::exists(path_to_vidpid)) {
      Log::Error() << "File " << path_to_vidpid << "doesn't exist.";
      return false;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't find file " << path_to_vidpid << "\n" << ex.what();
    return false;
  }
  std::stringstream buf;
  std::ifstream file_vid_pid(path_to_vidpid);
  if (!file_vid_pid.is_open()) {
    Log::Error() << "Can't open file" << path_to_vidpid;
    return false;
  }
  buf << file_vid_pid.rdbuf();
  file_vid_pid.close();

  boost::json::value json;
  boost::json::array *ptr_arr;
  try {
    json = boost::json::parse(buf.str());
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse a JSON file";
    return false;
  }
  ptr_arr = json.if_array();
  if (ptr_arr == nullptr || ptr_arr->empty()) {
    Log::Error() << "Empty json array";
    return false;
  }
  // for (auto it = ptr_arr->cbegin(); it != ptr_arr->cend(); ++it) {
  for (const auto &element : *ptr_arr) {
    if (element.is_object() && element.as_object().contains("vid") &&
        element.as_object().contains("pid")) {
      std::stringstream string_builder;
      string_builder
          << "block id "
          << UnQuote(element.as_object().at("vid").as_string().c_str()) << ":"
          << UnQuote(element.as_object().at("pid").as_string().c_str());
      try {
        rules_to_add.emplace_back(string_builder.str());
      } catch (const std::logic_error &ex) {
        Log::Warning() << "Error creating a rule from JSON";
        Log::Warning() << string_builder.str();
        return false;
      }
    }
  }
  return true;
}

std::optional<Target>
Guard::ExtractTargetPolicy(boost::json::object *p_obj) noexcept {
  if (p_obj != nullptr && p_obj->contains("policy_type") &&
      p_obj->at("policy_type").is_string()) {
    return p_obj->at("policy_type").as_string() == "radio_white_list"
               ? Target::block
               : Target::allow;
  }
  Log::Error() << "No target policy is found in JSON";
  return std::nullopt;
}

std::optional<bool>
Guard::ExtractDaemonTargetState(boost::json::object *p_obj) noexcept {
  if (p_obj != nullptr && p_obj->contains("run_daemon") &&
      p_obj->at("run_daemon").is_string()) {
    return p_obj->at("run_daemon").as_string() == "true";
  }
  Log::Error() << "No target daemon state is found in JSON";
  return std::nullopt;
}

std::optional<std::string>
Guard::ExtractPresetMode(boost::json::object *p_obj) noexcept {
  if (p_obj != nullptr && p_obj->contains("preset_mode") &&
      p_obj->at("preset_mode").is_string()) {
    return p_obj->at("preset_mode").as_string().c_str();
  }
  Log::Error() << "No preset mode is found in JSON";
  return std::nullopt;
}

} // namespace guard

// just in case
// this strings can be recieved from the rule, but they are not used yet
// std::string status =rule.targetToString(rule.getTarget());
// std::string device_id =rule.getDeviceID().toString();
// std::string name=rule.getName();
// std::string port = rule.getViaPort();
// int rule_id=rule.getRuleID();
// std::string conn_type=rule.getWithConnectType()
// std::string serial = rule.getSerial();
// std::string hash = rule.getHash();
// std::string par_hash = rule.getParentHash();
// std::string interface = rule.getWithConnectType();
// std::string withInterf= rule.attributeWithInterface().toRuleString();
// std::string cond=rule.attributeConditions().toRuleString();
//
// std::string label;
// try{
//    label =rule.getLabel();
// }
// catch(std::runtime_error& ex){
//     std::cerr << ex.what();
// }
