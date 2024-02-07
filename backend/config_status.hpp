#pragma once

#include "serializible_for_lisp.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <set>
#include <unordered_map>

#ifdef UNIT_TEST
#include "test.hpp"
#endif

namespace guard {

/************************************************************/

/**
 * @class ConfigStatus
 * @brief Status for configuration: suspiciuos udev rules,
 * and UsbGuard Status
 * */
class ConfigStatus : public SerializableForLisp<ConfigStatus> {

private:
  const std::string usb_guard_daemon_name = "usbguard.service";
  const std::string unit_dir_path = "/lib/systemd/system";
  const std::string usbguard_default_config_path =
      "/etc/usbguard/usbguard-daemon.conf";

public:
  // warning_info : filename
  std::unordered_map<std::string, std::string> udev_warnings;
  bool udev_rules_OK;
  bool guard_daemon_OK;
  bool guard_daemon_enabled;
  bool guard_daemon_active;
  std::string daemon_config_file_path;
  // filled by ParseDaemonConfig
  std::string daemon_rules_file_path;
  bool rules_files_exists;
  std::set<std::string> ipc_allowed_users;
  std::set<std::string> ipc_allowed_groups;

  /// @brief Constructor checks for udev rules and daemon status
  ConfigStatus();

  /// @brief Serialize statuses
  /// @return vector of string patrs
  vecPairs SerializeForLisp() const;

  /// @brief Checks the daemon status,fills status fields
  void CheckDaemon();

private:
  /// @brief Return path for the  daemon .conf file
  /// @return A string path, empty string if failed
  /// @details usbguard.service is expected to be installed in
  /// /lib/systemd/system
  std::string GetDaemonConfigPath() const;

  void ParseDaemonConfig();

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

/*************************************************************/
// non-friend funcions

/// @brief  inspect udev rules for suspicious files
/// @param vec just for testing purposes
/// @return map of string file:warning
std::unordered_map<std::string, std::string> InspectUdevRules(
#ifdef UNIT_TEST
    const std::vector<std::string> *vec = nullptr
#endif
);

} // namespace guard