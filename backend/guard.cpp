#include "guard.hpp"
#include "utils.hpp"
#include <filesystem>
#include <iostream>

namespace guard {



Guard::Guard() : ptr_ipc(nullptr) {
  try {
    ptr_ipc = std::make_unique<usbguard::IPCClient>(true);
  } catch (usbguard::Exception &e) {
    std::cerr << "Error connecting to USBGuard daemon \n"
              << e.what() << std::endl;
  }
}

bool Guard::HealthStatus() const { return ptr_ipc && ptr_ipc->isConnected(); }

std::vector<UsbDevice> Guard::ListCurrentUsbDevices() {
  std::vector<UsbDevice> res;
  if (!HealthStatus())
    return res;
  std::vector<usbguard::Rule> rules = ptr_ipc->listDevices(default_query);
  for (const usbguard::Rule &rule : rules) {
    res.emplace_back(rule.getRuleID(), rule.targetToString(rule.getTarget()),
                     rule.getName(), rule.getDeviceID().toString(),
                     rule.getViaPort(), rule.getWithConnectType());
    // std::cerr << rule.getRuleID() << ": " << rule.toString() << std::endl;
  }
  return res;
}

bool Guard::AllowOrBlockDevice(std::string id, bool allow, bool permanent) {
  if (id.empty() || !HealthStatus())
    return false;
  uint32_t id_numeric = StrToUint(id);
  if (!id_numeric)
    return false;
  usbguard::Rule::Target policy =
      allow ? usbguard::Rule::Target::Allow : usbguard::Rule::Target::Block;
  ptr_ipc->applyDevicePolicy(id_numeric, policy, permanent);
  return true;
}

ConfigStatus Guard::GetConfigStatus() {
  ConfigStatus config_status;

  // inspect udev rules for string constaining usb and authorized
  config_status.udev_warnings = InspectUdevRules();
  if (!config_status.udev_warnings.empty()) {
    config_status.udev_rules_OK = false;
  }
  
  // ConfigStatus
  //  TODO check status of Daemon (active + enabled)
  //  TODO if daemon is on active think about creating policy before enabling
  config_status.guard_daemon_OK=true;
  return config_status;
}

// ---------------------- private ---------------------------

std::unordered_map<std::string, std::string>
Guard::InspectUdevRules(
  #ifdef UNIT_TEST  
  const std::vector<std::string>* vec
  #endif
  ) {
  std::unordered_map<std::string, std::string> res;
  std::vector<std::string> udev_paths{
      "/usr/lib/udev/rules.d", "/usr/local/lib/udev/rules.d",
      "/run/udev/rules.d", "/etc/udev/rules.d"}; 
  #ifdef UNIT_TEST
     if (vec)
        udev_paths=*vec;
  #endif  
  for (const std::string &path : udev_paths) {
    std::cerr << "Inspecting udev folder " << path << std::endl;
    try {
      // find all files in folder
      std::vector<std::string> files = FindAllFilesInDirRecursive(path,".rules");

      // for each file - check if it contains suspicious strings
      for (const std::string &str_path : files) {
        std::ifstream f(str_path);
        if (f.is_open()) {
          std::string tmp_str;
          bool found_usb{false};
          bool found_authorize{false};
          // for each string
          while (getline(f, tmp_str)) {
            if (tmp_str.find("usb") != std::string::npos) {
              found_usb = true;
            }
            if (tmp_str.find("authorize") != std::string::npos) {
              found_authorize = true;
            }
          }
          tmp_str.clear();
          f.close();

          if (found_usb && found_authorize){
            std::cerr <<"Found file " << str_path <<std::endl;
              res.emplace( str_path,"usb_rule");
          }
        } else {
          std::cerr << "Can't open file " << str_path << std::endl;
        }
      }
    } catch (const std::exception &ex) {
      std::cerr << "Error checking " << path << std::endl;
      std::cerr << ex.what() << std::endl;
    }
  }
  return res;
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
