#include "config_status.hpp"
#include "systemd_dbus.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <string>

namespace guard {

/**********************************************************/
// ConfigStatus

ConfigStatus::ConfigStatus()
    : udev_warnings{InspectUdevRules()}, udev_rules_OK{udev_warnings.empty()},
      guard_daemon_OK{false}, guard_daemon_enabled{false},
      guard_daemon_active{false} {
  CheckDaemon();
}

vecPairs ConfigStatus::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("udev", udev_rules_OK ? "OK" : "BAD");
  res.emplace_back("usbguard", guard_daemon_OK ? "OK" : "BAD");
  res.emplace_back("usbguard_active",
                   guard_daemon_active ? "ACTIVE" : "STOPPED");
  res.emplace_back("usbguard_enabled",
                   guard_daemon_enabled ? "ENABLED" : "DISABLED");
  return res;
}

void ConfigStatus::CheckDaemon() {
  dbus_bindings::Systemd systemd;
  std::optional<bool> enabled = systemd.IsUnitEnabled(usb_guard_daemon_name);
  std::optional<bool> active = systemd.IsUnitActive(usb_guard_daemon_name);
  if (enabled.has_value())
    guard_daemon_enabled = enabled.value();
  else
    std::cerr << "[ERROR] Can't check if usbguard service is enabled"
              << std::endl;
  if (active.has_value())
    guard_daemon_active = active.value();
  else
    std::cerr << "[ERROR] Can't check if usbguard service is active"
              << std::endl;
  guard_daemon_OK = guard_daemon_enabled && guard_daemon_active;
}

std::string ConfigStatus::GetDamonConfigPath() const {
  std::string res;
  const std::string full_path_to_unit =
      unit_dir_path + "/" + usb_guard_daemon_name;
  try {
    // open unit (.service file)
    if (std::filesystem::exists(full_path_to_unit)) {
      std::ifstream f(full_path_to_unit);
      if (f.is_open()) {
        std::string line;
        // find ExecStart string
        while (getline(f, line)) {
          if (boost::algorithm::contains(line, "ExecStart")) {
            std::vector<std::string> tokens;
            // split by space, find next after -c
            boost::split(tokens, line, [](const char c) { return c == ' '; });
            auto it = std::find(tokens.begin(), tokens.end(), "-c");
            if (it != tokens.end())
              ++it;
            if (it != tokens.end() && !it->empty()) {
              boost::trim(*it);
              if (std::filesystem::exists(*it)) {
                res = std::move(*it);
                std::cerr << "[INFO] Found config file for usbguard " << res
                          << std::endl;
                break;
              } else {
                std::cerr << "[WARNING] Can't find path to .conf file in "
                             ".service file"
                          << std::endl;
                if (std::filesystem::exists(usbguard_default_config_path)) {
                  std::cerr << "[WARINIG]"
                            << "Default config found "
                            << usbguard_default_config_path << std::endl;
                  res = usbguard_default_config_path;
                  break;
                }
              }
            }
          }
          line.clear();
        }
        f.close();
      } else {
        std::cerr << "[ERROR] Can't read " << full_path_to_unit << std::endl;
      }
    }
  } catch (const std::exception &ex) {
    std::cerr << "[ERROR] Can't find usbguard config file" << std::endl;
    std::cerr << ex.what() << std::endl;
  }

  return res;
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
