#pragma once

#include "serializible_for_lisp.hpp"
#include "types.hpp"
#include "utils.hpp"
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
public:
  // warning_info : filename
  std::unordered_map<std::string, std::string> udev_warnings;
  bool udev_rules_OK;
  bool guard_daemon_OK;
  bool guard_daemon_enabled;
  bool guard_daemon_active;

  /// @brief Constructor checks for udev rules and daemon status
  ConfigStatus();

  /// @brief Serialize statuses
  /// @return vector of string patrs
  vecPairs SerializeForLisp() const;
  void CheckDaemon();

private:
  const std::string usb_guard_daemon_name = "usbguard.service";
};

/*************************************************************/
// non-friend funcions

/// @brief  inspect udev rules for suspicious files
/// @param vec just for testing purposes
/// @return map of string warning : file
std::unordered_map<std::string, std::string> InspectUdevRules(
#ifdef UNIT_TEST
    const std::vector<std::string> *vec = nullptr
#endif
);

} // namespace guard