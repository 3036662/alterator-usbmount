#pragma once
#include "usb_device.hpp"
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#ifdef UNIT_TEST
#include "test.hpp"
#endif

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

using RuleWithOptionalParam =
    std::pair<RuleConditions, std::optional<std::string>>;
using RuleWithBool = std::pair<bool, RuleWithOptionalParam>;

/**
 * @brief Parse and store rules from usbguard rules.conf file
 * @class GuardRule
 * @throws Constructor - std::logical_error
 */
class GuardRule {

  const std::map<Target, std::string> map_target{{Target::allow, "allow"},
                                                 {Target::block, "block"},
                                                 {Target::reject, "reject"}};
  const std::map<RuleConditions, std::string> map_conditions{
      {RuleConditions::localtime, "localtime"},
      {RuleConditions::allowed_matches, "allowed-matches"},
      {RuleConditions::rule_applied, "rule-applied"},
      {RuleConditions::rule_applied_past, "rule-applied("},
      {RuleConditions::rule_evaluated, "rule-evaluated"},
      {RuleConditions::rule_evaluated_past, "rule-evaluated("},
      {RuleConditions::random, "random"},
      {RuleConditions::random_with_propability, "random("},
      {RuleConditions::always_true, "true"},
      {RuleConditions::always_false, "false"},
      {RuleConditions::no_condition, ""}};

  const std::map<RuleOperator, std::string> map_operator{
      {RuleOperator::all_of, "all-of"},
      {RuleOperator::one_of, "one-of"},
      {RuleOperator::none_of, "none-of"},
      {RuleOperator::equals, "equals"},
      {RuleOperator::equals_ordered, "equals-ordered"},
      {RuleOperator::no_operator, ""}};

  const size_t usbguard_hash_length = 44;

public:
  int number; /// number of line (from the beginnig of file)
  Target target;
  std::optional<std::string> vid;
  std::optional<std::string> pid;
  std::optional<std::string> hash;
  std::optional<std::string> device_name;
  std::optional<std::string> serial;
  std::optional<std::pair<RuleOperator, std::vector<std::string>>> port;
  std::optional<std::pair<RuleOperator, std::vector<std::string>>>
      with_interface;
  std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>> cond;

  /**
   * @brief Construct a new Guard Rule object
   * @param str String rule from usbguard rules file
   * @throws std::logical_error
   */
  GuardRule(std::string raw_str);

private:
  /**
   * @brief Checks if sring looks like one of parameter reserve words for rule
   *
   * @param str parameter
   * @return true if string is normal value
   * @return false if string looks like reserved word
   */
  bool IsReservedWord(const std::string &str);

  /**
   * @brief Split string by space, dont split double-quouted substrings
   *
   * @param raw_str String to split
   * @return std::vector<std::string>
   * @example "id    1    name   "Long Name"" -> { "id","1",""Long Name""}
   */
  std::vector<std::string> SplitRawRule(std::string raw_str);

  /**
   * @brief Parse token
   *
   * @param splitted vector - a splitted rule string.
   * @param name String name of a parameter to look for.
   * @param predicat bool(sting&) function returning true, if string is valid
   * value
   * @return std::optional<std::string> value for parameter
   */
  std::optional<std::string>
  ParseToken(const std::vector<std::string> &splitted, const std::string &name,
             std::function<bool(const std::string &)> predicat) const;

  /**
   * @brief Parse token when operators are possible for token
   *
   * @param splitted vector - a splitted rule string.
   * @param name String name of a parameter to look for.
   * @param predicat bool(sting&) function returning true, if string is valid
   * value
   * @return std::optional<std::pair<RuleOperator, std::vector<std::string>>>
   * value for parameter
   */
  std::optional<std::pair<RuleOperator, std::vector<std::string>>>
  ParseTokenWithOperator(
      const std::vector<std::string> &splitted, const std::string &name,
      std::function<bool(const std::string &)> predicat) const;

  /**
   * @brief Parse conditions
   *
   * @param splitted Vector - a splitted rule string.
   * @return std::optional<
   * std::pair<RuleOperator, std::vector<std::pair<RuleConditions, bool>>>>
   */
  std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>>
  ParseConditions(const std::vector<std::string> &splitted);

  RuleWithBool ParseOneCondition(
      std::vector<std::string>::const_iterator &it_range_beg,
      std::vector<std::string>::const_iterator it_range_end) const;

  std::string ParseConditionParameter(
      std::vector<std::string>::const_iterator it_start,
      std::vector<std::string>::const_iterator it_end) const;
  bool CanConditionHaveParams(RuleConditions cond) const;

  RuleConditions ConvertToConditionWithParam(RuleConditions cond) const;

  std::string ConditionsToString() const;

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

} // namespace guard