#include "message_dispatcher.hpp"
#include "guard_rule.hpp"
#include "log.hpp"
#include "utils.hpp"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

using guard::utils::Log;

MessageDispatcher::MessageDispatcher(guard::Guard &guard) noexcept
    : guard(guard) {}

bool MessageDispatcher::Dispatch(const LispMessage &msg) noexcept {
  // list usbs
  if (msg.action == "list" && msg.objects == "list_curr_usbs") {
    return ListUsbDevices();
  }
  // allow device with id
  if (msg.action == "read" && msg.objects == "usb_allow") {
    return AllowDevice(msg);
  }
  // block device with id
  if (msg.action == "read" && msg.objects == "usb_block") {
    return BlockDevice(msg);
  }
  // get udev rules list
  if (msg.action == "list" && msg.objects == "check_config_udev") {
    return CheckConfig();
  }
  // get congiguration info
  if (msg.action == "read" && msg.objects == "config_status") {
    Log::Info() << "Get config status";
    std::cout << ToLispAssoc(guard.GetConfigStatus());
    return true;
  }
  // list usbguard rules
  if (msg.action == "list" && msg.objects == "list_rules" &&
      msg.params.count("level") > 0) {
    return ListUsbGuardRules(msg);
  }
  // save changes rules
  if (msg.action == "read" && msg.objects == "apply_changes") {
    return SaveChangeRules(msg);
  }
  // empty response
  std::cout << "(\n)\n";
  return true;
}

bool MessageDispatcher::SaveChangeRules(const LispMessage &msg) const noexcept {
  auto start = std::chrono::steady_clock::now();
  Log::Debug() << "Time measurement has started";
  if (msg.params.count("changes_json") == 0 ||
      msg.params.find("changes_json")->second.empty()) {
    std::cout << mess_beg << mess_end;
    Log::Warning() << "bad request for rules,doing nothing";
    return true;
  }
  std::string json_string = msg.params.at("changes_json");
  boost::replace_all(json_string, "\\\\\"", "\\\"");
  std::optional<std::string> result = guard.ParseJsonRulesChanges(json_string);
  if (result)
    *result = EscapeQuotes(*result);
  vecPairs vec_result;
  if (result) {
    vec_result.emplace_back("status", "OK");
    vec_result.emplace_back("ids_json", std::move(*result));
  } else {
    vec_result.emplace_back("status", "FAILED");
  }
  std::cout << ToLispAssoc(
      SerializableForLisp<vecPairs>(std::move(vec_result)));
  std::cout.flush();
  Log::Debug() << "[DEBUG] Elapsed(ms)=" << since(start).count();
  return true;
}

bool MessageDispatcher::ListUsbGuardRules(
    const LispMessage &msg) const noexcept {
  guard::StrictnessLevel level =
      guard::GuardRule::StrToStrictnessLevel(msg.params.at("level"));
  auto start = std::chrono::steady_clock::now();
  Log::Debug() << "Time measurement has started";
  std::vector<guard::GuardRule> vec_rules =
      guard.GetConfigStatus().ParseGuardRulesFile().first;
  std::cout << mess_beg;
  std::string response;
  // map vendor ids to strings
  if (level == guard::StrictnessLevel::vid_pid) {
    std::unordered_set<std::string> vendors;
    for (const auto &rule : vec_rules) {
      if (rule.vid)
        vendors.insert(rule.vid.value());
    }
    auto vendors_names = guard::MapVendorCodesToNames(vendors);
    for (auto &rule : vec_rules) {
      if (rule.vid.has_value() && vendors_names.count(rule.vid.value()) > 0) {
        rule.vendor_name = vendors_names.at(rule.vid.value());
      }
    }
  }
  for (const auto &rule : vec_rules) {
    if (rule.level == level) {
      response += ToLisp(rule);
    }
  }
  std::cout << response << mess_end;
  Log::Debug() << "Elapsed(ms)=" << since(start).count();
  return true;
}

bool MessageDispatcher::ListUsbDevices() const noexcept {
  Log::Debug() << "Time measurement has started";
  std::vector<guard::UsbDevice> vec_usb = guard.ListCurrentUsbDevices();
  std::cout << mess_beg;
  for (const auto &usb : vec_usb) {
    std::cout << ToLisp(usb);
  }
  std::cout << mess_end;
  return true;
}

bool MessageDispatcher::AllowDevice(const LispMessage &msg) const noexcept {
  if (msg.params.count("usb_id") == 0 ||
      msg.params.find("usb_id")->second.empty()) {
    std::cout << mess_beg << mess_end;
    Log::Warning() << "Bad request for usb allow,doing nothing";
    return true;
  }
  if (guard.AllowOrBlockDevice(msg.params.find("usb_id")->second, true)) {
    std::cout << mess_beg << "status" << WrapWithQuotes("OK") << mess_end;
  } else {
    std::cout << mess_beg << "status" << WrapWithQuotes("FAIL") << mess_end;
  }
  return true;
}

bool MessageDispatcher::BlockDevice(const LispMessage &msg) const noexcept {
  if (msg.params.count("usb_id") == 0 ||
      msg.params.find("usb_id")->second.empty()) {
    std::cout << mess_beg << mess_end;
    Log::Warning() << "Bad request for usb allow,doing nothing";
    return true;
  }
  if (guard.AllowOrBlockDevice(msg.params.find("usb_id")->second, false)) {
    std::cout << mess_beg << "status" << WrapWithQuotes("OK") << mess_end;
  } else {
    std::cout << mess_beg << "status" << WrapWithQuotes("FAIL") << mess_end;
  }
  return true;
}

bool MessageDispatcher::CheckConfig() const noexcept {
  Log::Info() << "Check config";
  std::string str = mess_beg;
  for (const auto &pair : guard.GetConfigStatus().udev_warnings) {
    str += ToLisp("label_udev_rules_filename", pair.first);
  }
  str += mess_end;
  std::cout << str;
  return true;
}