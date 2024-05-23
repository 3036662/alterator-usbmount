#include "config_status.hpp"
#include "common_utils.hpp"
#include "guard_audit.hpp"
#include "guard_utils.hpp"
#include "log.hpp"
#include "systemd_dbus.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace guard {
using common_utils::FindAllFilesInDirRecursive;
using common_utils::Log;

ConfigStatus::ConfigStatus() noexcept
    : udev_warnings_{utils::InspectUdevRules()},
      udev_rules_OK_{udev_warnings_.empty()}, guard_daemon_OK{false},
      guard_daemon_enabled_{false}, guard_daemon_active_{false},
      config_file_permissions_OK_(false), rules_file_permissions_OK_(false),
      daemon_config_file_path_{GetDaemonConfigPath()},
      rules_files_exists_(false), audit_backend_(AuditType::kUndefined) {
  CheckDaemon();
  ParseDaemonConfig();
  CheckConfigFilesPermissions();
}

vecPairs ConfigStatus::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("udev", udev_rules_OK_ ? "OK" : "BAD");
  res.emplace_back("usbguard", guard_daemon_OK ? "OK" : "BAD");
  res.emplace_back("usbguard_active",
                   guard_daemon_active_ ? "ACTIVE" : "STOPPED");
  res.emplace_back("usbguard_enabled",
                   guard_daemon_enabled_ ? "ENABLED" : "DISABLED");
  res.emplace_back("rules_file_exists", rules_files_exists_ ? "TRUE" : "FALSE");
  res.emplace_back("allowed_users", boost::join(ipc_allowed_users_, ", "));
  res.emplace_back("allowed_groups", boost::join(ipc_allowed_groups_, ", "));
  res.emplace_back("implicit_policy", implicit_policy_target_);
  res.emplace_back(
      "config_files_permissions",
      config_file_permissions_OK_ && rules_file_permissions_OK_ ? "OK" : "BAD");
  res.emplace_back("audit_type",
                   audit_backend_ == AuditType::kFileAudit ? "file" : "audit");
  return res;
}

void ConfigStatus::CheckDaemon() noexcept {
  dbus_bindings::Systemd systemd;
  std::optional<bool> enabled = systemd.IsUnitEnabled(usb_guard_daemon_name);
  std::optional<bool> active = systemd.IsUnitActive(usb_guard_daemon_name);
  if (enabled.has_value())
    guard_daemon_enabled_ = enabled.value_or(false);
  else
    Log::Error() << "Can't check if usbguard service is enabled";
  if (active.has_value())
    guard_daemon_active_ = active.value_or(false);
  else
    Log::Error() << "Can't check if usbguard service is active";
  guard_daemon_OK = guard_daemon_enabled_ && guard_daemon_active_;
}

std::string ConfigStatus::GetDaemonConfigPath() const noexcept {
  std::string res;
  const std::string full_path_to_unit =
      unit_dir_path + "/" + usb_guard_daemon_name;
  try {
    // open unit (.service file)
    if (std::filesystem::exists(full_path_to_unit)) {
      std::ifstream file_unit(full_path_to_unit);
      if (file_unit.is_open()) {
        std::string line;
        // find ExecStart string
        while (getline(file_unit, line)) {
          std::optional<std::string> val = ExtractConfigFileName(line);
          if (val.has_value()) {
            res = val.value();
            break;
          }
          line.clear();
        }
        file_unit.close();
      } else {
        Log::Error() << "Can't read " << full_path_to_unit;
      }
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't find usbguard config file";
    Log::Error() << ex.what();
  }
  return res;
}

std::optional<std::string>
ConfigStatus::ExtractConfigFileName(const std::string &line) const noexcept {
  using common_utils::Log;
  std::string res;
  if (!boost::algorithm::contains(line, "ExecStart"))
    return std::nullopt;
  std::vector<std::string> tokens;
  // split by space, find next after -c
  try {
    boost::split(tokens, line,
                 [](const char symbol) noexcept { return symbol == ' '; });
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
  }
  auto it_token = std::find(tokens.begin(), tokens.end(), "-c");
  if (it_token != tokens.end())
    ++it_token;
  if (it_token != tokens.end() && !it_token->empty()) {
    *it_token = boost::trim_copy(*it_token); // strong ex guarantee
    try {
      if (std::filesystem::exists(*it_token)) {
        res = std::move(*it_token);
        // Log::Info() << "Found config file for usbguard " << res;
      } else {
        Log::Warning() << "Can't find path to .conf file in .service file";

        if (std::filesystem::exists(usbguard_default_config_path)) {
          Log::Warning() << "Default config found "
                         << usbguard_default_config_path;
          res = usbguard_default_config_path;
        }
      }
    } catch (const std::exception &ex) {
      Log::Error() << "Filesystem error";
      Log::Error() << ex.what();
    }
  }
  return res;
}

bool ConfigStatus::ExtractRuleFilePath(const std::string &line) noexcept {
  if (boost::starts_with(line, "RuleFile=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string path_to_rules(line, pos);
      boost::trim(path_to_rules);
      // Log::Info() << "Find path to rules file " << path_to_rules;
      if (!path_to_rules.empty()) {
        daemon_rules_file_path = std::move(path_to_rules);
        try {
          rules_files_exists_ = std::filesystem::exists(daemon_rules_file_path);
        } catch (const std::exception &ex) {
          Log::Error() << "Can't check if rules file " << daemon_rules_file_path
                       << " exists";
          return false;
        }
        return true;
      }
    }
  }
  return false;
}

bool ConfigStatus::ExtractUsers(const std::string &line) noexcept {
  if (boost::starts_with(line, "IPCAllowedUsers=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string users_string(line, pos);
      users_string = boost::trim_copy(users_string);
      // Log::Info() << "Found users in conf file " << line;
      // split by space and add to set
      std::vector<std::string> splitted_string;
      try {
        boost::split(splitted_string, users_string,
                     [](const char symbol) { return symbol == ' '; });
      } catch (const std::exception &ex) {
        Log::Error() << ex.what();
      }
      for (std::string &str : splitted_string) {
        boost::trim(str);
        if (!str.empty()) {
          ipc_allowed_users_.insert(str);
        }
      }
    }
    return true;
  }
  return false;
}

bool ConfigStatus::CheckUserFiles(const std::string &line) noexcept {
  if (boost::starts_with(line, "IPCAccessControlFiles=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string path_to_folder(line, pos);
      boost::trim(path_to_folder);
      //  Log::Info() << "Found users control folder " << line;
      try {
        std::filesystem::path fs_path_to_folder(path_to_folder);
        std::vector<std::string> files =
            FindAllFilesInDirRecursive({path_to_folder, ""});
        for (std::string &str : files) {
          boost::trim(str);
          if (!str.empty()) {
            ipc_allowed_users_.insert(
                std::filesystem::path(str).filename().string());
          }
        }
      } catch (const std::exception &ex) {
        Log::Error() << "Can't check users control folder" << path_to_folder;
        Log::Error() << ex.what();
      }
    }
    return true;
  }
  return false;
}

bool ConfigStatus::ExtractGroups(const std::string &line) noexcept {
  if (boost::starts_with(line, "IPCAllowedGroups=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::vector<std::string> splitted_string;
      std::string groups_string(line, pos);
      try {
        boost::split(splitted_string, groups_string,
                     [](const char symbol) { return symbol == ' '; });
      } catch (const std::exception &ex) {
        Log::Error() << ex.what();
      }
      for (std::string &str : splitted_string) {
        boost::trim(str);
        if (!str.empty()) {
          ipc_allowed_groups_.insert(str);
        }
      }
    }
    return true;
  }
  return false;
}

bool ConfigStatus::ExtractPolicy(const std::string &line) noexcept {
  if (boost::starts_with(line, "ImplicitPolicyTarget=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string implicit_target(line, pos);
      boost::trim(implicit_target);
      if (!implicit_target.empty()) {
        implicit_policy_target_ = std::move(implicit_target);
        Log::Info() << "Implicit policy target = " << implicit_policy_target_;
      }
    }
    return true;
  }
  return false;
}

bool ConfigStatus::ExtractAuditBackend(const std::string &line) noexcept {
  if (boost::starts_with(line, "AuditBackend=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string audit_backend(line, pos);
      boost::trim(audit_backend);
      if (!audit_backend.empty()) {
        // Log::Info() << "Audit backend type =" << audit_backend;
        if (audit_backend == "FileAudit") {
          audit_backend_ = AuditType::kFileAudit;
          return true;
        }
        if (audit_backend == "LinuxAudit") {
          audit_backend_ = AuditType::kLinuxAudit;
          return true;
        }
        audit_backend_ = AuditType::kUndefined;
        Log::Error() << "AuditType was not parsed";
      }
    }
    return true;
  }
  return false;
}

bool ConfigStatus::ExtractAuditFilePath(const std::string &line) noexcept {
  if (boost::starts_with(line, "AuditFilePath=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string audit_file(line, pos);
      boost::trim(audit_file);
      if (!audit_file.empty()) {
        // Log::Info() << "Audit file =" << audit_file;
        audit_file_path_ = std::move(audit_file);
      } else {
        Log::Warning() << "Audit file path is empty";
      }
    }
    return true;
  }
  return false;
}

void ConfigStatus::ParseDaemonConfig() noexcept {
  // cleanup
  ipc_allowed_users_.clear();
  ipc_allowed_groups_.clear();
  daemon_rules_file_path.clear();
  rules_files_exists_ = false;
  if (daemon_config_file_path_.empty())
    return;
  // open config
  std::ifstream file_dconfig(daemon_config_file_path_);
  if (!file_dconfig.is_open()) {
    Log::Error() << "Can't open daemon config file "
                 << daemon_config_file_path_;
    return;
  }
  // parse
  std::string line;
  while (getline(file_dconfig, line)) {
    boost::trim(line);
    // rule file path
    if (ExtractRuleFilePath(line))
      continue;
    // parse allowed user from conf file
    if (ExtractUsers(line))
      continue;
    // parse allowed users folder and find all files in folder
    if (CheckUserFiles(line))
      continue;
    // find all groups in config file
    if (ExtractGroups(line))
      continue;
    // Audit type
    if (ExtractAuditBackend(line))
      continue;
    if (ExtractAuditFilePath(line))
      continue;
    // ImplicitPolicyTarget
    ExtractPolicy(line);
  }
  file_dconfig.close();
}

void ConfigStatus::CheckConfigFilesPermissions() noexcept {
  namespace fs = std::filesystem;
  try {
    fs::path config_path(daemon_config_file_path_);
    if (fs::exists(config_path)) {
      fs::perms conf_perm = fs::status(config_path).permissions();
      if (conf_perm == (fs::perms::owner_write | fs::perms::owner_read)) {
        config_file_permissions_OK_ = true;
      } else {
        config_file_permissions_OK_ = false;
        Log::Error() << "USBGuard config file (" << daemon_config_file_path_
                     << ") must have 0600 permissions";
      }
    }
    if (fs::exists(daemon_rules_file_path)) {
      fs::perms conf_perm = fs::status(daemon_rules_file_path).permissions();
      if (conf_perm == (fs::perms::owner_write | fs::perms::owner_read)) {
        rules_file_permissions_OK_ = true;
      } else {
        rules_file_permissions_OK_ = false;
        Log::Error() << "USBGuard rules file (" << daemon_rules_file_path
                     << ") must have 0600 permissions";
      }
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't check file permissions for USBGuard files";
    Log::Error() << ex.what();
  }
}

std::pair<std::vector<GuardRule>, uint>
ConfigStatus::ParseGuardRulesFile() const noexcept {
  std::pair<std::vector<GuardRule>, uint> res;
  res.second = 0;
  try {
    if (!std::filesystem::exists(daemon_rules_file_path)) {
      Log::Warning() << "The rules file for usbguard doesn't exist.";
      return res;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse rules file " << daemon_rules_file_path;
    return res;
  }
  std::ifstream file(daemon_rules_file_path);
  if (!file.is_open()) {
    Log::Error() << "Can't open file " << daemon_rules_file_path;
    return res;
  }
  // Parse the file
  std::string line;
  uint counter = 0;
  uint counter_fails = 0;
  while (std::getline(file, line)) {
    try {
      res.first.emplace_back(GuardRule(line));
      res.first.back().number(counter);
      ++counter;
    } catch (const std::logic_error &ex) {
      Log::Error() << "Can't parse the rule " << line;
      ++counter_fails;
    }
    line.clear();
  }
  file.close();

  res.second = counter + counter_fails;
  Log::Info() << "Parsed " << res.first.size() << " rules."
              << " Failed " << counter_fails;
  return res;
}

bool ConfigStatus::OverwriteRulesFile(const std::string &new_content,
                                      bool run_daemon) noexcept {
  // temporary change the policy to "allow all"
  // The purpose is to launch USBGuard without blocking anything
  // to make sure it can parse new rules
  // bool initial_policy = implicit_policy_target_ == "block";
  // if (!ChangeImplicitPolicy(false)) {
  //   Log::Error() << "Can't change usbguard policy";
  //   return false;
  // }
  if (rules_files_exists_) {
    // read old rules
    std::ifstream old_file(daemon_rules_file_path);
    if (!old_file.is_open()) {
      Log::Error() << "[OverwriteRulesFile] Can't open file "
                   << daemon_rules_file_path;
      return false;
    }
    std::stringstream old_content;
    old_content << old_file.rdbuf();
    old_file.close();

    // write new rules
    std::ofstream file(daemon_rules_file_path);
    if (!file.is_open()) {
      Log::Error() << "Can't open file " << daemon_rules_file_path
                   << " for writing";
      return false;
    }
    file << new_content;
    file.close();
    // parse new rules
    // if (!new_content.empty()) {
    auto parsed_new_file = ParseGuardRulesFile();
    // If some errors occurred while parsing new rules - recover
    if (parsed_new_file.first.size() != parsed_new_file.second ||
        !TryToRun(run_daemon)) {
      Log::Error() << "[OverwriteRulesFile] Error parsing new rules,old file "
                      "will be recovered";
      std::ofstream file3(daemon_rules_file_path);
      if (!file3.is_open()) {
        Log::Error() << "[OverwriteRulesFile] Can't open file "
                     << daemon_rules_file_path << " for writing";
        return false;
      }
      file3 << old_content.str();
      file3.close();
      dbus_bindings::Systemd sysd;
      // try to run with recovered rules
      if (!TryToRun(run_daemon)) {
        Log::Error() << "[OverwriteRulesFile] Can't start usbguard service. "
                        "UsbGuard will be disabled";
        ChangeImplicitPolicy(false);
        // TryToRun(false);
        ChangeDaemonStatus(false, false);
      }
      return false;
    }
    //}
  }

  // restore the policy
  // if (!ChangeImplicitPolicy(initial_policy)) {
  //   Log::Error() << "Can't change usbguard policy";
  //   return false;
  // // }
  // if (run_daemon)
  //  TryToRun(run_daemon);

  return true;
}

bool ConfigStatus::TryToRun(bool run_daemon) const noexcept {
  dbus_bindings::Systemd sysd;
  // check if unit is active
  auto init_state = sysd.IsUnitActive(usb_guard_daemon_name);
  if (!init_state.has_value())
    return false;
  Log::Info() << "[TryToRun] Usbguard is "
              << (init_state.value_or("") ? "active" : "inactive");
  // if stopped - try to start and exit
  if (!init_state.value_or(false)) {
    auto result = sysd.StartUnit(usb_guard_daemon_name);
    Log::Info() << "[TryToRun] Test run - "
                << ((result.has_value() && *result) ? "OK" : "FAIL");
    if (!run_daemon) {
      Log::Debug() << "[TryToRun] stopping service";
      sysd.StopUnit(usb_guard_daemon_name);
    } else {
      StartUsbguardDbus(true, sysd);
    }
    return result.value_or(false);
  }
  //  else if daemon is active - restart
  // auto result = sysd.RestartUnit(usb_guard_daemon_name);
  // The restart method is commented out because it can cause a systemd timeout.
  // Using stop and start.
  auto result_stop = sysd.StopUnit(usb_guard_daemon_name);
  if (!result_stop.value_or(false)) {
    Log::Error() << "[TryToRun] Can't stop service while trying to restart";
  }
  auto result = sysd.StartUnit(usb_guard_daemon_name);
  Log::Info() << "[TryToRun] Restart - "
              << (result.value_or(false) ? "OK" : "FAIL");
  if (init_state.has_value() && !init_state.value_or(false) && !run_daemon) {
    sysd.StopUnit(usb_guard_daemon_name);
  } else {
    StartUsbguardDbus(true, sysd);
  }
  return result.value_or(false);
}

std::optional<GuardAudit> ConfigStatus::GetAudit() const noexcept {
  try {
    return std::make_optional<GuardAudit>(audit_backend_, audit_file_path_);
  } catch (const std::exception &ex) {
    Log::Error() << "Can't get GuardAudit";
    return std::nullopt;
  }
}

bool ConfigStatus::StartUsbguardDbus(
    bool start, dbus_bindings::Systemd &sysd) const noexcept {
  std::optional<bool> dbus_start_res;
  if (start) {
    Log::Debug() << "[StartUsbguardDbus] Starting usbguard-dbus service";
    dbus_start_res = sysd.StartUnit(usb_guard_dbus_daemon_name);
    Log::Debug() << "[StartUsbguardDbus] "
                 << (dbus_start_res.value_or(false) ? "OK" : "FAILED");
  } else {
    Log::Debug() << "[StopUsbguardDbus] Stoping usbguard-dbus service";
    dbus_start_res = sysd.StopUnit(usb_guard_dbus_daemon_name);
    Log::Debug() << "[StopUsbguardDbus] "
                 << (dbus_start_res.value_or(false) ? "OK" : "FAILED");
  }
  return dbus_start_res.value_or(false);
}

bool ConfigStatus::EnableUsbguardDbus(
    bool enable, dbus_bindings::Systemd &sysd) const noexcept {
  std::optional<bool> dbus_enable_res;
  if (enable) {
    Log::Debug() << "[EnableUsbguardDbus] Enabling usbguard-dbus service";
    dbus_enable_res = sysd.EnableUnit(usb_guard_dbus_daemon_name);
    Log::Debug() << "[EnableUsbguardDbus] "
                 << (dbus_enable_res.value_or(false) ? "OK" : "FAILED");
  } else {
    Log::Debug() << "[DisableUsbguardDbus] Disabling usbguard-dbus service";
    dbus_enable_res = sysd.DisableUnit(usb_guard_dbus_daemon_name);
    Log::Debug() << "[DisableUsbguardDbus] "
                 << (dbus_enable_res.value_or(false) ? "OK" : "FAILED");
  }
  return dbus_enable_res.value_or(false);
}

bool ConfigStatus::ChangeDaemonStatus(bool active,
                                      bool enabled) const noexcept {
  dbus_bindings::Systemd sysd;
  auto init_state = sysd.IsUnitActive(usb_guard_daemon_name);
  if (!init_state.has_value())
    return false;
  Log::Info() << "[ChangeDaemonStatus] Usbguard is "
              << (*init_state ? "active" : "inactive");
  auto enabled_state = sysd.IsUnitEnabled(usb_guard_daemon_name);
  if (!enabled_state) {
    Log::Error() << "Can't define if USBGuard enabled or disabled";
    return false;
  }
  Log::Info() << "[ChangeDaemonStatus] Usbguard is "
              << (*enabled_state ? "enabled" : "disabled");
  // if we need to stop the service
  if (*init_state && !active) {
    Log::Info() << "[ChangeDaemonStatus] Stopping the service";
    auto res = sysd.StopUnit(usb_guard_daemon_name);
    if (!res || !*res) {
      Log::Error() << "[ChangeDaemonStatus] Can't stop the USBGuard";
      return false;
    }
  } else if (!*init_state && active) {
    Log::Info() << "Starting the service";
    auto res = sysd.StartUnit(usb_guard_daemon_name);
    if (!res || !*res) {
      Log::Error() << "[ChangeDaemonStatus] Can't start the USBGuard";
      return false;
    }
    StartUsbguardDbus(true, sysd);
  }
  // if now the daemon is enabled and we need to disable it
  if (*enabled_state && !enabled) {
    Log::Info() << "[ChangeDaemonStatus] Disabling the service";
    auto res = sysd.DisableUnit(usb_guard_daemon_name);
    if (!res || !*res) {
      Log::Error() << "Can't disable the USBGuard";
      return false;
    }
    EnableUsbguardDbus(false, sysd);
  } else if (!*enabled_state && enabled) {
    auto res = sysd.EnableUnit(usb_guard_daemon_name);
    Log::Info() << "Enabling the service";
    if (!res || !*res) {
      Log::Error() << "[ChangeDaemonStatus] Can't enable the USBGuard";
      return false;
    }
    EnableUsbguardDbus(true, sysd);
  }
  return true;
}

bool ConfigStatus::ChangeImplicitPolicy(bool block) noexcept {
  if (block && implicit_policy_target_ == "block")
    return true;
  if (!block && implicit_policy_target_ == "allow")
    return true;
  Log::Debug() << "Changing implicit policy to " << (block ? "block" : "allow");
  try {
    if (!std::filesystem::exists(daemon_config_file_path_)) {
      Log::Error() << "Config file " << daemon_config_file_path_
                   << "doesn't exist.";
      return false;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Error looking for config file at path "
                 << daemon_config_file_path_;
    Log::Error() << ex.what();
    return false;
  }
  std::string new_target = block ? "block" : "allow";
  // open config
  std::ifstream file_config(daemon_config_file_path_);
  if (!file_config.is_open()) {
    Log::Error() << "Can't open daemon config file "
                 << daemon_config_file_path_;
    return false;
  }
  std::stringstream new_content;
  std::stringstream old_content;
  std::string line;
  while (getline(file_config, line)) {
    old_content << line << "\n";
    boost::trim(line);
    if (boost::starts_with(line, "ImplicitPolicyTarget=")) {
      size_t pos = line.find('=');
      if (pos != std::string::npos && ++pos < line.size()) {
        line.erase(pos, line.size() - pos);
        line += new_target;
      }
    }
    new_content << line << "\n";
    line.clear();
  }
  file_config.close();
  // write to config
  std::ofstream f_out(daemon_config_file_path_);
  if (!f_out.is_open()) {
    Log::Error() << "Can't open file " << daemon_config_file_path_
                 << " for writing";
    return false;
  }
  f_out << new_content.str();
  f_out.close();
  ParseDaemonConfig();
  return true;
}

} // namespace guard