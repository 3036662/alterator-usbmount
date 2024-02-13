#include "message_dispatcher.hpp"
#include "utils.hpp"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include "guard_rule.hpp"
#include <chrono>

MessageDispatcher::MessageDispatcher(guard::Guard &guard) : guard(guard) {}

bool MessageDispatcher::Dispatch(const LispMessage &msg) {
  std::cerr << msg << std::endl;
  //std::cerr << "curr action " << msg.action << std::endl;

  // list usbs
  if (msg.action == "list" && msg.objects == "list_curr_usbs") {
    auto start = std::chrono::steady_clock::now();
    std::cerr << "[INFO] Time measurement has started"<<std::endl;
    std::vector<guard::UsbDevice> vec_usb = guard.ListCurrentUsbDevices();
    std::cout << mess_beg;
    for (const auto &usb : vec_usb) {
      std::cout << ToLisp(usb);
    }
    std::cout << mess_end;
    std::cerr << "[INFO] Elapsed(ms)=" << since(start).count() << std::endl;
    return true;
  }

  // allow device with id
  if (msg.action == "read" && msg.objects == "usb_allow") {
    if (!msg.params.count("usb_id") ||
        msg.params.find("usb_id")->second.empty()) {
      std::cout << mess_beg << mess_end;
      std::cerr << "bad request for usb allow,doing nothing" << std::endl;
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
      std::cerr << "bad request for usb allow,doing nothing" << std::endl;
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
    std::cerr << "[Dispatcher] Check config" << std::endl;
    std::string str = mess_beg;
    for (const auto &pair : guard.GetConfigStatus().udev_warnings) {
      str += ToLisp("label_udev_rules_filename", pair.first);
    }
    str += mess_end;
    std::cout << str;
    return true;
  }

  if (msg.action == "read" && msg.objects == "config_status") {
    std::cerr << "[Dispatcher] Get config status" << std::endl;
    std::cout << ToLispAssoc(guard.GetConfigStatus());
    return true;
  }


  // list usbguard rules
  if (msg.action == "list" && msg.objects == "list_rules" && msg.params.count("level")) {
    guard::StrictnessLevel level=guard::StrToStrictnessLevel(msg.params.at("level"));
    auto start = std::chrono::steady_clock::now();
    std::cerr << "[INFO] Time measurement has started"<<std::endl;
    std::vector<guard::GuardRule> vec_rules = guard.GetConfigStatus().ParseGuardRulesFile();
    std::cout << mess_beg;
    std::string response;
    // map vendor ids to strings 
    if (level == guard::StrictnessLevel::vid_pid){
      std::unordered_set<std::string> vendors;
      for (const auto& r:vec_rules){
        if (r.vid)
        vendors.insert(r.vid.value());
      }
      auto vendors_names=guard.MapVendorCodesToNames(vendors);
      for  (auto& r:vec_rules){
        if (r.vid.has_value() && vendors_names.count(r.vid.value())){
          r.vendor_name=vendors_names.at(r.vid.value());
        } 
      }
    }

    for (const auto &rule : vec_rules) {
      if (rule.level==level){
        response += ToLisp(rule);
      }
    }
    std::cout <<response<< mess_end;
    std::cerr << "[INFO]Elapsed(ms)=" << since(start).count() << std::endl;
    return true;
  }

  // empty response
  std::cout << "(\n)\n";
  return true;
}
