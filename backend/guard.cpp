#include "guard.hpp"
#include "log.hpp"
#include "utils.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/json.hpp>
#include <unordered_set>

namespace guard {

using guard::utils::Log;

/******************************************************************************/
Guard::Guard() : ptr_ipc(nullptr) { ConnectToUsbGuard(); }

/******************************************************************************/

bool Guard::HealthStatus() const { return ptr_ipc && ptr_ipc->isConnected(); }

/******************************************************************************/

std::vector<UsbDevice> Guard::ListCurrentUsbDevices() {
  std::vector<UsbDevice> res;
  if (!HealthStatus())
    return res;
  // health is ok -> get all devices
  std::vector<usbguard::Rule> rules = ptr_ipc->listDevices(default_query);
  for (const usbguard::Rule &rule : rules) {
    std::vector<std::string> i_types =
        FoldUsbInterfacesList(rule.attributeWithInterface().toRuleString());

    // TODO create for each type
    for (const std::string &i_type : i_types) {
      std::vector<std::string> vid_pid;
      boost::split(vid_pid, rule.getDeviceID().toString(),
                   [](const char c) { return c == ':'; });
      res.emplace_back(rule.getRuleID(), rule.targetToString(rule.getTarget()),
                       rule.getName(), vid_pid.size() > 0 ? vid_pid[0] : "",
                       vid_pid.size() > 1 ? vid_pid[1] : "", rule.getViaPort(),
                       rule.getWithConnectType(), i_type, rule.getSerial(),
                       rule.getHash());
    }
  }

  // fill all vendor names with one read of file
  std::unordered_set<std::string> vendors_to_search;
  for (const UsbDevice &usb : res) {
    vendors_to_search.insert(usb.vid);
  }
  std::unordered_map<std::string, std::string> vendors_names{
      MapVendorCodesToNames(vendors_to_search)};
  for (UsbDevice &usb : res) {
    if (vendors_names.count(usb.vid)) {
      usb.vendor_name = vendors_names[usb.vid];
    }
  }

  return res;
}

/******************************************************************************/

bool Guard::AllowOrBlockDevice(const std::string &device_id, bool allow,
                               bool permanent) {
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

/******************************************************************************/

ConfigStatus Guard::GetConfigStatus() {
  ConfigStatus config_status;
  //  TODO if daemon is on active think about creating policy before enabling
  if (!HealthStatus())
    ConnectToUsbGuard();
  config_status.guard_daemon_active = HealthStatus();
  return config_status;
}

/******************************************************************************/

std::optional<std::vector<guard::GuardRule>>
Guard::DeleteRules(const std::vector<uint> &rule_indexes) noexcept {
  ConfigStatus cs = GetConfigStatus();

  // make sure that old rules were successfully parsed
  std::pair<std::vector<guard::GuardRule>, uint> parsed_rules =
      cs.ParseGuardRulesFile();
  if (parsed_rules.first.size() != parsed_rules.second) {
    Log::Error() << "The rules file is not completelly parsed, can't edit";
    return std::nullopt;
  }

  // copy old rules,except listed in rule_indexes
  std::set<uint> unique_indexes(rule_indexes.cbegin(), rule_indexes.cend());
  std::vector<guard::GuardRule> new_rules;
  for (const auto &rule : parsed_rules.first) {
    if (!unique_indexes.count(rule.number))
      new_rules.push_back(rule);
  }
  return new_rules;
}

/******************************************************************************/
// private

void Guard::ConnectToUsbGuard() noexcept {
  try {
    ptr_ipc = std::make_unique<usbguard::IPCClient>(true);
  } catch (usbguard::Exception &e) {
    Log::Warning() << "Error connecting to USBGuard daemon.";
    Log::Warning() << e.what();
  }
}

/******************************************************************************/

std::vector<std::string>
Guard::FoldUsbInterfacesList(std::string i_type) const {
  boost::erase_first(i_type, "with-interface");
  boost::trim(i_type);
  std::vector<std::string> vec_i_types;
  // if a multiple types in one string
  if (i_type.find('{') != std::string::npos &&
      i_type.find('}') != std::string::npos) {
    std::vector<std::string> splitted;
    boost::erase_all(i_type, "{");
    boost::erase_all(i_type, "}");
    boost::trim(i_type);
    boost::split(splitted, i_type, [](const char c) { return c == ' '; });
    // create sequence of usb types
    std::vector<UsbType> vec_usb_types;
    for (const std::string &el : splitted) {
      try {
        vec_usb_types.emplace_back(el);
      } catch (const std::exception &e) {
        Log::Error() << "Can't parse a usb type" << el;
        Log::Error() << e.what();
      }
    }
    // fold if possible
    // put to multiset bases
    std::multiset<char> set;
    for (const UsbType &usb_type : vec_usb_types) {
      set.emplace(usb_type.base);
    }
    // if a key is not unique, create a mask.
    auto it_unique_end = std::unique(
        vec_usb_types.begin(), vec_usb_types.end(),
        [](const UsbType &a, const UsbType &b) { return a.base == b.base; });
    for (auto it = vec_usb_types.begin(); it != it_unique_end; ++it) {
      size_t n = set.count(it->base);
      std::string tmp = it->base_str;
      if (n == 1) {
        tmp += ':';
        tmp += it->sub_str;
        tmp += ':';
        tmp += it->protocol_str;
      } else {
        tmp += ":*:*";
      }
      vec_i_types.emplace_back(std::move(tmp));
    }
  } else {
    vec_i_types.push_back(std::move(i_type));
  }
  return vec_i_types;
}

/******************************************************************************/

std::unordered_map<std::string, std::string> Guard::MapVendorCodesToNames(
    const std::unordered_set<std::string> &vendors) const {
  std::unordered_map<std::string, std::string> res;
  const std::string path_to_usb_ids = "/usr/share/misc/usb.ids";
  try {
    if (std::filesystem::exists(path_to_usb_ids)) {
      std::ifstream f(path_to_usb_ids);
      if (f.is_open()) {
        std::string line;
        while (getline(f, line)) {
          // not interested in strings starting with tab
          if (!line.empty() && line[0] == '\t') {
            line.clear();
            continue;
          }
          auto range = boost::find_first(line, "  ");
          if (range) {
            std::string vendor_id(line.begin(), range.begin());
            if (vendors.count(vendor_id)) {
              std::string vendor_name(range.end(), line.end());
              res.emplace(std::move(vendor_id), std::move(vendor_name));
            }
          }
          line.clear();
        }
        f.close();
      } else {
        Log::Warning() << "Can't open file " << path_to_usb_ids;
      }
    } else {
      Log::Error() << "The file " << path_to_usb_ids << "doesn't exist";
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't map vendor IDs to vendor names.";
    Log::Error() << ex.what();
  }
  return res;
}
/******************************************************************************/

std::optional<std::string>
Guard::ParseJsonRulesChanges(const std::string &msg) noexcept {

  std::string preset_mode;
  bool daemon_activate{false};
  namespace json = boost::json;
  // parse the json and process
  json::object *ptr_jobj = nullptr;
  json::value json_value;
  bool rules_changed_by_policy;
  try {
    json_value = json::parse(msg);
    ptr_jobj = json_value.if_object();
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse JSON";
    Log::Error() << ex.what();
  }

  // daemon target state (active or stopped)
  if (ptr_jobj && ptr_jobj->contains("run_daemon") &&
      ptr_jobj->at("run_daemon").if_string()) {
    daemon_activate = ptr_jobj->at("run_daemon").as_string() == "true";
  } else {
    Log::Error() << "No target daemon state is found in JSON";
    return std::nullopt;
  }

  // policy type
  Target policy;
  if (ptr_jobj && ptr_jobj->contains("policy_type") &&
      ptr_jobj->at("policy_type").if_string()) {
    policy = ptr_jobj->at("policy_type").as_string() == "radio_white_list"
                 ? Target::block
                 : Target::allow;
  } else {
    Log::Error() << "No target policy is found in JSON";
    return std::nullopt;
  }

  // preset mode
  if (ptr_jobj && ptr_jobj->contains("preset_mode") &&
      ptr_jobj->at("preset_mode").if_string()) {
    preset_mode = ptr_jobj->at("preset_mode").as_string().c_str();
  } else {
    Log::Error() << "No preset mode is found in JSON";
    return std::nullopt;
  }

  // new rules will be stored here
  std::vector<guard::GuardRule> new_rules;
  std::vector<uint> rules_to_delete;
  std::vector<GuardRule> rules_to_add;
  auto cs = GetConfigStatus();

  // result json
  boost::json::object obj_result;

  // manual mode
  if (preset_mode == "manual_mode") {
    obj_result = ProcessJsonManualMode(ptr_jobj, rules_to_delete, rules_to_add);
    // if some rules are bad - just return a validation result
    if (!obj_result.at("rules_BAD").as_array().empty()) {
      return json::serialize(std::move(obj_result));
    }
    // build a vector with new rules
    auto to_new_rules = DeleteRules(rules_to_delete);
    if (!to_new_rules.has_value()) {
      Log::Error() << "Can't delete rules";
      return std::nullopt;
    }
    // skip rules with conflicting policy,place rest to new_rules
    // if implicit policy="allow" rules must be blow or reject
    for (GuardRule &rule : *to_new_rules) {
      if ((policy == Target::allow &&
           (rule.target == Target::block || rule.target == Target::reject)) ||
          (policy == Target::block && rule.target == Target::allow)) {
        new_rules.emplace_back(std::move(rule));
      } else {
        rules_changed_by_policy = true;
      }
    }
    // change policy (bool)
    cs.ChangeImplicitPolicy(policy == Target::block);
  }

  // block all except connected
  if (preset_mode == "put_connected_to_white_list" ||
      preset_mode == "put_connected_to_white_list_plus_HID") {
    obj_result = ProcessJsonAllowConnected(rules_to_add);
  }
  // add HID devices to white list
  if (preset_mode == "put_connected_to_white_list_plus_HID") {
    AddAllowHid(rules_to_add);
  }

  // block only usb mass storage  and MTP PTP
  if (preset_mode == "put_disks_and_mtp_to_black") {
    AddBlockUsbStorages(rules_to_add);
    cs.ChangeImplicitPolicy(false);
    obj_result["STATUS"] = "OK";
  }

  // block known android devices
  if (preset_mode == "block_and_reject_android") {
    if (!AddRejectAndroid(rules_to_add))
      return std::nullopt;
    cs.ChangeImplicitPolicy(false);
    obj_result["STATUS"] = "OK";
  }

  // add rules_to_add to new rules vector
  for (auto &r : rules_to_add)
    new_rules.push_back(std::move(r));
  // build one string for file overwrite
  std::string str_new_rules;
  for (const auto &r : new_rules) {
    str_new_rules += r.BuildString(true, true);
    str_new_rules += "\n";
  }

  // overwrite the rules and test launch if some rules were added or deleted
  if (rules_changed_by_policy || !rules_to_delete.empty() ||
      !rules_to_add.empty()) {
    if (!cs.OverwriteRulesFile(str_new_rules, daemon_activate)) {
      Log::Error() << "Can't launch the daemon with new rules";
      return std::nullopt;
    }
  }
  if (!cs.ChangeDaemonStatus(daemon_activate, daemon_activate)) {
    Log::Error() << "Change the daemon status FAILED";
  }
  return json::serialize(std::move(obj_result));
}

boost::json::object
Guard::ProcessJsonManualMode(const boost::json::object *ptr_jobj,
                             std::vector<uint> &rules_to_delete,
                             std::vector<GuardRule> &rules_to_add) noexcept {
  namespace json = boost::json;
  boost::json::array json_arr_OK;
  boost::json::array json_arr_BAD;
  // if some new rules were added
  // put new rules to vector rules_to_add
  // put rules ids to json_arr_OK and json_arr_BAD
  if (ptr_jobj && ptr_jobj->contains("appended_rules")) {
    const json::array *ptr_json_array_rules =
        ptr_jobj->at("appended_rules").if_array();
    if (ptr_json_array_rules && !ptr_json_array_rules->empty()) {
      // for each rule
      for (auto it = ptr_json_array_rules->cbegin();
           it != ptr_json_array_rules->cend(); ++it) {
        const json::object *ptr_json_rule = it->if_object();
        if (ptr_json_rule) {
          const json::string *tr_id = ptr_json_rule->at("tr_id").if_string();
          // try to build a rule
          try {
            GuardRule rule{ptr_json_rule};
            rules_to_add.push_back(std::move(rule));
            if (tr_id && !tr_id->empty()) {
              json_arr_OK.emplace_back(*tr_id);
            }
          }
          // if failed
          catch (const std::logic_error &ex) {
            Log::Error() << "Can't build the rule";
            Log::Error() << ex.what();
            if (tr_id && !tr_id->empty()) {
              json_arr_BAD.emplace_back(*tr_id);
            }
          }
        }
      }
    }
  }
  // if we need to remove some rules
  // put rules numbers to "rules_to_delete"
  if (ptr_jobj->contains("deleted_rules") &&
      ptr_jobj->at("deleted_rules").is_array()) {
    for (const auto &el : ptr_jobj->at("deleted_rules").as_array()) {
      if (!el.if_string())
        continue;
      auto id = StrToUint(el.as_string().c_str());
      if (id) {
        rules_to_delete.push_back(*id);
      }
    }
  }
  // put list of validated string to response json
  json::object obj_result;
  obj_result["rules_OK"] = std::move(json_arr_OK);
  obj_result["rules_BAD"] = std::move(json_arr_BAD);
  obj_result["STATUS"] =
      obj_result.at("rules_BAD").as_array().empty() ? "OK" : "BAD";
  return obj_result;
}

boost::json::object Guard::ProcessJsonAllowConnected(
    std::vector<GuardRule> &rules_to_add) noexcept {
  namespace json = boost::json;
  json::object res;
  auto cs = GetConfigStatus();
  // temporary change the policy to "allow all"
  // The purpose is to launch USBGuard without blocking anything
  // to receive a list of devices
  if (!cs.ChangeImplicitPolicy(false)) {
    Log::Error() << "Can't change usbguard policy";
    res["STATUS"] = "error";
    res["ERROR_MSG"] = "Failed to change usbguard policy";
    return res;
  }
  Log::Info() << "USBguard implicit policy was changed to allow all";
  // make sure that USBGuard is running
  ConnectToUsbGuard();
  if (!HealthStatus()) {
    cs.TryToRun(true);
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
    std::stringstream ss;
    ss << "allow name " << QuoteIfNotQuoted(dev.name) << " hash "
       << QuoteIfNotQuoted(dev.hash);
    try {
      rules_to_add.emplace_back(ss.str());
    } catch (const std::logic_error &ex) {
      Log::Error() << "Can't create a rule for device"
                   << "allow name " << QuoteIfNotQuoted(dev.name) << " hash "
                   << QuoteIfNotQuoted(dev.hash);
      Log::Error() << ex.what();
      res["STATUS"] = "error";
      res["ERROR_MSG"] = "Failed to create policy.Failed";
      return res;
    }
  }

  if (cs.implicit_policy_target != "block") {
    Log::Info() << "Changing USBGuard policy to block all";
    cs.ChangeImplicitPolicy(true);
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
  std::ifstream f(path_to_vidpid);
  if (!f.is_open()) {
    Log::Error() << "Can't open file" << path_to_vidpid;
    return false;
  }
  buf << f.rdbuf();
  f.close();

  boost::json::value json;
  boost::json::array *ptr_arr;
  try {
    json = boost::json::parse(buf.str());
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse a JSON file";
    return false;
  }
  ptr_arr = json.if_array();
  if (!ptr_arr || ptr_arr->empty()) {
    Log::Error() << "Empty json array";
    return false;
  }
  for (auto it = ptr_arr->cbegin(); it != ptr_arr->cend(); ++it) {
    if (it->if_object() && it->as_object().contains("vid") &&
        it->as_object().contains("pid")) {
      std::stringstream ss;
      ss << "block id "
         << UnQuote(it->as_object().at("vid").as_string().c_str()) << ":"
         << UnQuote(it->as_object().at("pid").as_string().c_str());
      try {
        rules_to_add.emplace_back(ss.str());
      } catch (const std::logic_error &ex) {
        Log::Warning() << "Error creating a rule from JSON";
        Log::Warning() << ss.str();
        return false;
      }
    }
  }
  return true;
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
