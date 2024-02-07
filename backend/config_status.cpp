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
      guard_daemon_active{false},
      daemon_config_file_path{GetDaemonConfigPath()} {
  CheckDaemon();
  ParseDaemonConfig();
}

vecPairs ConfigStatus::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("udev", udev_rules_OK ? "OK" : "BAD");
  res.emplace_back("usbguard", guard_daemon_OK ? "OK" : "BAD");
  res.emplace_back("usbguard_active",
                   guard_daemon_active ? "ACTIVE" : "STOPPED");
  res.emplace_back("usbguard_enabled",
                   guard_daemon_enabled ? "ENABLED" : "DISABLED");
  res.emplace_back("rules_file_exists",rules_files_exists ? "TRUE" : "FALSE" ); 

  res.emplace_back("allowed_users",boost::join(ipc_allowed_users,", "));
  res.emplace_back("allowed_groups",boost::join(ipc_allowed_groups,", "));

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

std::string ConfigStatus::GetDaemonConfigPath() const {
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

void ConfigStatus::ParseDaemonConfig() {

  // cleanup
  ipc_allowed_users.clear();
  ipc_allowed_groups.clear();
  daemon_rules_file_path.clear();
  rules_files_exists = false;
  if (daemon_config_file_path.empty())
    return;

  // open config
  std::ifstream f(daemon_config_file_path);
  if (!f.is_open()) {
    std::cerr << "[ERROR] Can't open daemon config file "
              << daemon_config_file_path << std::endl;
    return;
  }

  // parse
  std::string line;
  while (getline(f, line)) {
    boost::trim(line);
    // rule file path
    if (boost::starts_with(line, "RuleFile=")) {
      size_t pos = line.find('=');
      if (pos != std::string::npos && ++pos < line.size()) {
        std::string path_to_rules(line, pos);
        boost::trim(path_to_rules);
        std::cerr << "[INFO] Find path to rules file " << path_to_rules
                  << std::endl;
        if (!path_to_rules.empty()) {
          daemon_rules_file_path = std::move(path_to_rules);
          try {
            rules_files_exists =
                std::filesystem::exists(daemon_rules_file_path);
          } catch (const std::exception &ex) {
            std::cerr << "[ERROR] Can't check if rules file "
                      << daemon_rules_file_path << " exists" << std::endl;
          }
        }
      }
      continue;
    }

    // parse allowed user from conf file
    if (boost::starts_with(line, "IPCAllowedUsers=")) {
      size_t pos = line.find('=');
      if (pos != std::string::npos && ++pos < line.size()) {
        std::string users_string(line, pos);
        boost::trim(users_string);
        std::cerr << "[INFO] Found users in conf file " << line << std::endl;
        // split by space and add to set
        std::vector<std::string> splitted_string;
        boost::split(splitted_string, users_string,
                     [](const char c) { return c == ' '; });
        for (std::string &str : splitted_string) {
          boost::trim(str);
          if (!str.empty()) {
            ipc_allowed_users.insert(str);
          }
        }
      }
      continue;
    }

    // parse allowed users folder and find all files in folder
    if (boost::starts_with(line, "IPCAccessControlFiles=")) {
      size_t pos = line.find('=');
      if (pos != std::string::npos && ++pos < line.size()) {
        std::string path_to_folder(line, pos);
        boost::trim(path_to_folder);
        std::cerr << "[INFO] Found users control folder " << line << std::endl;
        try {
          std::filesystem::path fs_path_to_folder(path_to_folder);
          std::vector<std::string> files =
              FindAllFilesInDirRecursive(path_to_folder);
          for (std::string &str : files) {
            boost::trim(str);
            if (!str.empty()) {
              // std::cerr << "found file " << str <<std::endl;
              ipc_allowed_users.insert(
                  std::filesystem::path(str).filename().string());
            }
          }
        } catch (const std::exception &ex) {
          std::cerr << "Can't check users control folder" << path_to_folder
                    << std::endl;
          std::cerr << ex.what();
        }
      }
      continue;
    }

    // find all groups in config file
    if (boost::starts_with(line, "IPCAllowedGroups=")) {
      size_t pos = line.find('=');
      if (pos != std::string::npos && ++pos < line.size()) {
        std::vector<std::string> splitted_string;
        std::string groups_string(line, pos);
        boost::split(splitted_string, groups_string,
                     [](const char c) { return c == ' '; });
        for (std::string &str : splitted_string) {
          boost::trim(str);
          if (!str.empty()) {
            ipc_allowed_groups.insert(str);
          }
        }
      }
      continue;
    }
    line.clear();
  }

  f.close();
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
    std::cerr << "[INFO] Inspecting udev folder " << path << std::endl;
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

          //if (found_usb && found_authorize) {
         //a rule can ruin program behavior even if only authorized and no usb
         if (found_authorize){   
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
