#pragma once

#include <unordered_map>
#include "serializible_for_lisp.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace guard {


/**
 * @class ConfigStatus
 * @brief Status for configuration: suspiciuos udev rules,
 * and UsbGuard Status
 * */
class ConfigStatus:public SerializableForLisp<ConfigStatus> {
public:
  bool udev_rules_OK = false;
  std::unordered_map<std::string, std::string>
      udev_warnings; /// warning_info : filename
  bool guard_daemon_OK = false;

  /// @brief Serialize statuses
  /// @return vector of string patrs
  vecPairs SerializeForLisp() const;
};

}