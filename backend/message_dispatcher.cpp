#include "message_dispatcher.hpp"
#include "guard_rule.hpp"
#include "log.hpp"
#include "utils.hpp"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

using guard::utils::Log;

MessageDispatcher::MessageDispatcher(guard::Guard &guard) : guard(guard) {}

bool MessageDispatcher::Dispatch(const LispMessage &msg) {
  // std::cerr << msg << std::endl;
  //  std::cerr << "curr action " << msg.action << std::endl;

  // list usbs
  if (msg.action == "list" && msg.objects == "list_curr_usbs") {
    auto start = std::chrono::steady_clock::now();
    Log::Debug() << "Time measurement has started";
    std::vector<guard::UsbDevice> vec_usb = guard.ListCurrentUsbDevices();
    std::cout << mess_beg;
    for (const auto &usb : vec_usb) {
      std::cout << ToLisp(usb);
    }
    std::cout << mess_end;
    Log::Debug() << "Elapsed(ms)=" << since(start).count();
    return true;
  }

  // allow device with id
  if (msg.action == "read" && msg.objects == "usb_allow") {
    if (!msg.params.count("usb_id") ||
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

  // block device with id
  if (msg.action == "read" && msg.objects == "usb_block") {
    if (!msg.params.count("usb_id") ||
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

  // get udev rules list
  if (msg.action == "list" && msg.objects == "check_config_udev") {
    Log::Info() << "Check config";
    std::string str = mess_beg;
    for (const auto &pair : guard.GetConfigStatus().udev_warnings) {
      str += ToLisp("label_udev_rules_filename", pair.first);
    }
    str += mess_end;
    std::cout << str;
    return true;
  }

  if (msg.action == "read" && msg.objects == "config_status") {
    Log::Info() << "Get config status";
    std::cout << ToLispAssoc(guard.GetConfigStatus());
    return true;
  }

  // list usbguard rules
  if (msg.action == "list" && msg.objects == "list_rules" &&
      msg.params.count("level")) {
    guard::StrictnessLevel level =
        guard::StrToStrictnessLevel(msg.params.at("level"));
    auto start = std::chrono::steady_clock::now();
    Log::Debug() << "Time measurement has started";
    std::vector<guard::GuardRule> vec_rules =
        guard.GetConfigStatus().ParseGuardRulesFile().first;
    std::cout << mess_beg;
    std::string response;
    // map vendor ids to strings
    if (level == guard::StrictnessLevel::vid_pid) {
      std::unordered_set<std::string> vendors;
      for (const auto &r : vec_rules) {
        if (r.vid)
          vendors.insert(r.vid.value());
      }
      auto vendors_names = guard.MapVendorCodesToNames(vendors);
      for (auto &r : vec_rules) {
        if (r.vid.has_value() && vendors_names.count(r.vid.value())) {
          r.vendor_name = vendors_names.at(r.vid.value());
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

  // save changes rules
  if (msg.action == "read" && msg.objects == "apply_changes") {
    auto start = std::chrono::steady_clock::now();
    Log::Debug() << "Time measurement has started";
    if (!msg.params.count("changes_json") ||
        msg.params.find("changes_json")->second.empty()) {
      std::cout << mess_beg << mess_end;
      Log::Warning() << "bad request for rules,doing nothing";
      return true;
    }
    std::string json_string = msg.params.at("changes_json");
    boost::replace_all(json_string, "\\\\\"", "\\\"");
    std::optional<std::string> result =
        guard.ParseJsonRulesChanges(json_string);
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

  // empty response
  std::cout << "(\n)\n";
  return true;
}
