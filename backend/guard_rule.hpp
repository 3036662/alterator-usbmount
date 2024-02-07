#pragma once
#include "usb_device.hpp"
#include <boost/algorithm/string.hpp>
#include <string>
#include <unordered_set>
#include <vector>

namespace guard {

/// @brief Target ::= allow | block | reject
enum class Target { allow, block, reject };

// clang-format off

/**
 * @brief Operators used in usbguard rules
 *
 */
enum class RuleOperator {
  all_of,  // Evaluate to true if all of the specified conditions evaluated to true.
  one_of,  // Evaluate to true if one of the specified conditions evaluated to true.
  none_of, // Evaluate to true if none of the specified conditions evaluated to true.
  equals,  //  Same as all-of.
  equals_ordered, //  Same as all-of.
  no_operator     // if no operators are used
};

/**
 * @brief Conditions used in usbguard rules
 *
 */
enum class RuleConditions {
  localtime,    //  Evaluates to true if the local time is in the specified time range.
  allowed_matches, // Evaluates to true if an allowed device matches the specified query.
  rule_applied, // Evaluates to true if the rule currently being evaluated ever matched a device.
  rule_applied_past, // Evaluates to true if the rule currently being evaluatedmatched a device in the past duration of time specified by the parameter.
  rule_evaluated, // Evaluates to true if the rule currently being evaluated was ever evaluated before.
  rule_evaluated_past, // Evaluates to true if the rule currently being evaluated was evaluated in the pas duration of timespecified by the parameter.
  random,       // Evaluates to true/false with a probability of p=0.5
  random_with_propability, // Evaluates to true with the specified probability p_true.
  always_true,  //  Evaluates always to true.
  always_false, // Evaluates always to false
  no_condition
};

// clang-format on

/**
 * @brief Parse and store rules from usbguard rules.conf file
 * @class GuardRule
 * @throws Constructor - std::logical_error
 */
class GuardRule {
public:
  int number; /// number of line (from the beginnig of file)
  Target target;
  std::string vid;
  std::string pid;
  std::string hash;
  std::string device_name;
  std::string serial;
  std::pair<RuleOperator, std::vector<std::string>> port;
  std::pair<RuleOperator, std::vector<UsbType>> with_interface;
  std::pair<RuleOperator, std::vector<std::pair<RuleConditions, bool>>> cond;

  /**
   * @brief Construct a new Guard Rule object
   * @param str String rule from usbguard rules file
   * @throws std::logical_error
   */
  GuardRule(const std::string &str);
};

} // namespace guard