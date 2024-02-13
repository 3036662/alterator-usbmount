#include "guard.hpp"
#include "utils.hpp"
#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace guard {

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

bool Guard::AllowOrBlockDevice(std::string id, bool allow, bool permanent) {
  if (id.empty() || !HealthStatus())
    return false;
  uint32_t id_numeric = StrToUint(id);
  if (!id_numeric)
    return false;
  usbguard::Rule::Target policy =
      allow ? usbguard::Rule::Target::Allow : usbguard::Rule::Target::Block;
  try {
    ptr_ipc->applyDevicePolicy(id_numeric, policy, permanent);
  } catch (const usbguard::Exception &ex) {
    std::cerr << "[ERROR] Can't add rule."
              << "May be rule conflict happened" << std::endl
              << ex.what() << std::endl;
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
// private

void Guard::ConnectToUsbGuard() noexcept {
  try {
    ptr_ipc = std::make_unique<usbguard::IPCClient>(true);
  } catch (usbguard::Exception &e) {
    std::cerr << "Error connecting to USBGuard daemon \n"
              << e.what() << std::endl;
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
        std::cerr << "[ERROR] Can't parse a usb type" << el << std::endl;
        std::cerr << e.what() << '\n';
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
    const std::unordered_set<std::string> vendors) const {
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
        std::cerr << "[WARNING] Can't open file" << path_to_usb_ids
                  << std::endl;
      }
    } else {
      std::cerr << "[WARNING] The file " << path_to_usb_ids << "doesn't exist";
    }
  } catch (const std::exception &ex) {
    std::cerr << "[ERROR] Can't map vendor IDs to vendor names." << std::endl
              << ex.what() << std::endl;
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
