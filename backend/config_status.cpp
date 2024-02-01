#include "config_status.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <sdbus-c++/sdbus-c++.h>

namespace guard {

/**********************************************************/
// ConfigStatus

ConfigStatus::ConfigStatus()
    : udev_warnings{InspectUdevRules()}, udev_rules_OK{udev_warnings.empty()},
      guard_daemon_OK{false} {
  // TODO initialize guard_daemon_OK
}

vecPairs ConfigStatus::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("udev", udev_rules_OK ? "OK" : "BAD");
  res.emplace_back("usbguard", guard_daemon_OK ? "OK" : "BAD");
  return res;
}

void ConfigStatus::CheckDaemon(){
  //std::unique_ptr<sdbus::IConnection>  connection = sdbus::createSystemBusConnection();
  const std::string destinationName = "org.freedesktop.systemd1";
  const std::string objectPath = "/org/freedesktop/systemd1";
  const std::string interfaceName ="org.freedesktop.systemd1.Manager";
  std::unique_ptr<sdbus::IProxy> concatenatorProxy = 
                      sdbus::createProxy(destinationName, objectPath);
  auto method = concatenatorProxy->createMethodCall(interfaceName, "GetUnitFileState");
  method << "usbguard.service";
  auto reply = concatenatorProxy->callMethod(method);  
  std::string result;  
  reply >> result;
  std::cerr <<result;
  // TODO - check active status too 
  //"org.freedesktop.systemd1.Unit", "ActiveState";

}

/***********************************************************/
// non-friend funcions

std::unordered_map<std::string, std::string> InspectUdevRules(
#ifdef UNIT_TEST
    const std::vector<std::string> *vec
#endif
) {
  std::unordered_map<std::string, std::string> res;
  std::vector<std::string> udev_paths{"/usr/lib/udev/rules.d",
                                      "/usr/local/lib/udev/rules.d",
                                      "/run/udev/rules.d", "/etc/udev/rules.d"};
#ifdef UNIT_TEST
  if (vec)
    udev_paths = *vec;
#endif
  for (const std::string &path : udev_paths) {
    std::cerr << "Inspecting udev folder " << path << std::endl;
    try {
      // find all files in folder
      std::vector<std::string> files =
          FindAllFilesInDirRecursive(path, ".rules");

      // for each file - check if it contains suspicious strings
      for (const std::string &str_path : files) {
        std::ifstream f(str_path);
        if (f.is_open()) {
          std::string tmp_str;
          bool found_usb{false};
          bool found_authorize{false};
          // for each string
          while (getline(f, tmp_str)) {
            // case insentitive search
            std::transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(),
                           [](unsigned char c) {
                             return std::isalnum(c) ? std::tolower(c) : c;
                           });

            if (tmp_str.find("usb") != std::string::npos) {
              found_usb = true;
            }
            if (tmp_str.find("authorize") != std::string::npos) {
              found_authorize = true;
            }
          }
          tmp_str.clear();
          f.close();

          if (found_usb && found_authorize) {
            std::cerr << "Found file " << str_path << std::endl;
            res.emplace(str_path, "usb_rule");
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