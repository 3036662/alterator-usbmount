#pragma once

#include <unordered_map>
#include "serializible_for_lisp.hpp"
#include "types.hpp"
#include "utils.hpp"

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
class ConfigStatus:public SerializableForLisp<ConfigStatus> {
public:
  // warning_info : filename
  std::unordered_map<std::string, std::string> udev_warnings; 
  bool udev_rules_OK;
  bool guard_daemon_OK;

  /// @brief Constructor checks for udev rules and daemon status
  ConfigStatus();

  /// @brief Serialize statuses
  /// @return vector of string patrs
  vecPairs SerializeForLisp() const;
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



}