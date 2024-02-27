#include "guard_rule.hpp"
#include "log.hpp"
#include "utils.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/json.hpp>
#include <functional>
#include <map>
#include <sstream>

namespace guard {

using guard::utils::Log;
using ::utils::EscapeQuotes;
using ::utils::QuoteIfNotQuoted;

/******************************************************************************/
// static
const std::map<Target, std::string> GuardRule::map_target{
    {Target::allow, "allow"},
    {Target::block, "block"},
    {Target::reject, "reject"}};
const std::map<RuleConditions, std::string> GuardRule::map_conditions{
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
const std::map<RuleOperator, std::string> GuardRule::map_operator{
    {RuleOperator::all_of, "all-of"},
    {RuleOperator::one_of, "one-of"},
    {RuleOperator::none_of, "none-of"},
    {RuleOperator::equals, "equals"},
    {RuleOperator::equals_ordered, "equals-ordered"},
    {RuleOperator::no_operator, ""}};

/******************************************************************************/
GuardRule::GuardRule(const std::string &raw_str) {
  std::logic_error ex_common("Cant parse rule string");
  // Split string to tokens.
  std::vector<std::string> splitted{SplitRawRule(raw_str)};
  if (splitted.empty())
    throw ex_common;
  // Map strings to values/
  // The target is mandatory.
  auto it_target = std::find_if(map_target.cbegin(), map_target.cend(),
                                [&splitted](const auto &element) {
                                  return element.second == splitted[0];
                                });
  if (it_target == map_target.cend()) {
    throw ex_common;
  }
  target = it_target->first;
  splitted.erase(splitted.begin());
  if (splitted.size() == 1)
    return;
  // id (vid:pid)
  // token id MUST contain a ':' symbol
  std::optional<std::string> str_id =
      ParseToken(splitted, "id", VidPidValidator);

  if (str_id) {
    size_t separator = str_id->find(':');
    vid = str_id->substr(0, separator);
    pid = str_id->substr(separator + 1);
    if (vid->empty() || pid->empty())
      throw ex_common;
  }

  // Hash, device_name, serial, port, and with_interface may or may not exist in
  // the string. default_predicat - Function is the default behavior of values
  // validating.

  // The default predicat for validation
  std::function<bool(const std::string &)> default_predicat =
      [](const std::string &val) { return !IsReservedWord(val); };
  // hash length MUST be > 10 symbols
  hash = ParseToken(splitted, "hash",
                    [](const std::string &val) { return val.size() > 10; });
  parent_hash = ParseToken(splitted, "parent-hash", [](const std::string &val) {
    return val.size() > 10;
  });
  device_name = ParseToken(splitted, "name", default_predicat);
  serial = ParseToken(splitted, "serial", default_predicat);
  port = ParseTokenWithOperator(splitted, "via-port", default_predicat);

  // if a rules contains  with-interface { i1, i2 } array - insert the "equals"
  // operator
  auto it_with_interface =
      std::find(splitted.begin(), splitted.end(), "with-interface");
  if (it_with_interface != splitted.end()) {
    ++it_with_interface;
    if (it_with_interface != splitted.end() && *it_with_interface == "{") {
      splitted.insert(it_with_interface, "equals");
    }
  }
  with_interface =
      ParseTokenWithOperator(splitted, "with-interface", InterfaceValidator);

  conn_type = ParseToken(splitted, "with-connect-type", default_predicat);
  // Conditions may or may not exist in the string.
  cond = ParseConditions(splitted);

  // check
  for (std::string &str : splitted)
    boost::trim(str);
  auto it_end =
      std::remove_if(splitted.begin(), splitted.end(),
                     [](const std::string &str) { return str.empty(); });
  splitted.erase(it_end, splitted.end());
  if (!splitted.empty()) {
    Log::Error() << "Not all token were parsed in the rule";
    Log::Error err;
    for (const auto &tok : splitted) {
      err << "token" << tok << " ";
    }
    throw std::logic_error("Not all tokens were parsed");
  }
  // Determine the stricness level
  if (hash)
    level = StrictnessLevel::hash;
  else if (vid && pid)
    level = StrictnessLevel::vid_pid;
  else if (with_interface)
    level = StrictnessLevel::interface;
  else
    level = StrictnessLevel::non_strict;
}

/******************************************************************************/
void GuardRule::WrapBracesWithSpaces(std::string &raw_str) {
  std::string tmp;
  for (auto it = raw_str.begin(); it != raw_str.end(); ++it) {
    bool wrap =
        *it == '{' || *it == '}' || *it == '!' || *it == '(' || *it == ')';
    if (wrap) {
      tmp.push_back(' ');
    }
    tmp.push_back(*it);
    if (wrap) {
      tmp.push_back(' ');
    }
  }
  std::swap(raw_str, tmp);
}

std::vector<std::string> GuardRule::SplitRawRule(std::string raw_str) noexcept {
  boost::trim(raw_str);
  std::vector<std::string> res;
  if (raw_str.empty())
    return res;
  WrapBracesWithSpaces(raw_str);
  // Split string with spaces.
  // I current space is a part of a quoted string - skip it.
  auto it_slow = raw_str.begin();
  auto it_fast = it_slow;
  ++it_fast;
  bool dont_split{raw_str[0] == '\"'};
  while (it_fast != raw_str.end()) {
    if (*it_fast == ' ' && !dont_split) {
      res.emplace_back(it_slow, it_fast);
      while (it_fast != raw_str.end() && *it_fast == ' ')
        ++it_fast;
      it_slow = it_fast;
    }
    if (it_fast != raw_str.end() && *it_fast == '\"') {
      dont_split = !dont_split;
    }
    if (it_fast != raw_str.end())
      ++it_fast;
  }
  if (it_fast == raw_str.end() && !dont_split && it_slow < it_fast)
    res.emplace_back(it_slow, it_fast);
  for (std::string &str : res)
    boost::trim(str);
  for (auto it = res.begin(); it != res.end(); ++it) {
    if (it->empty())
      res.erase(it);
  }
  return res;
}

/******************************************************************************/

bool GuardRule::IsReservedWord(const std::string &str) noexcept {
  if (str.empty())
    return false;
  if (*str.cbegin() == '\"')
    return false;
  if (std::find_if(map_target.cbegin(), map_target.cend(),
                   [&str](const auto &pair) { return pair.second == str; }) !=
      map_target.cend())
    return true;
  if (std::find_if(map_conditions.cbegin(), map_conditions.cend(),
                   [&str](const auto &pair) { return pair.second == str; }) !=
      map_conditions.cend())
    return true;
  if (std::find_if(map_operator.cbegin(), map_operator.cend(),
                   [&str](const auto &pair) { return pair.second == str; }) !=
      map_operator.cend())
    return true;
  return false;
}

/******************************************************************************/

std::optional<std::string> GuardRule::ParseToken(
    std::vector<std::string> &splitted, const std::string &name,
    const std::function<bool(const std::string &)> &predicat) {

  std::optional<std::string> res;
  auto it_name = std::find(splitted.cbegin(), splitted.cend(), name);
  if (it_name != splitted.cend()) {
    auto it_name_param = it_name;
    ++it_name_param;
    if (it_name_param != splitted.cend() && predicat(*it_name_param)) {
      res = *it_name_param;
    } else {
      Log::Error() << "Parsing error, token " << *it_name_param;
      throw std::logic_error("Cant parse rule string");
    }
    splitted.erase(it_name, ++it_name_param);
  }
  return res;
}

std::vector<std::string>::const_iterator GuardRule::ParseCurlyBracesArray(
    std::vector<std::string>::const_iterator it_range_begin,
    std::vector<std::string>::const_iterator it_end,
    const std::function<bool(const std::string &)> &predicat,
    std::vector<std::string> &res_array) {
  std::logic_error ex_common("Cant parse rule string");
  ++it_range_begin;
  // if no array found -> throw exception
  if (it_range_begin == it_end || *it_range_begin != "{") {
    // Log::Error() << "Error parsing values for " << name << " param.";
    throw ex_common;
  }
  auto it_range_end = std::find(it_range_begin, it_end, "}");
  if (it_range_end == it_end) {
    Log::Error() << "A closing \"}\" expectend for sequence.";
    throw ex_common;
  }
  if (std::distance(it_range_begin, it_range_end) == 1) {
    throw std::logic_error("Empty array {} is not supported");
  }
  // fill result with values
  auto it_val = it_range_begin;
  ++it_val;
  while (it_val != it_range_end) {
    if (predicat(*it_val)) {
      res_array.emplace_back(*it_val);
    } else {
      Log::Error() << "Parsing error, token " << *it_val;
      throw ex_common;
    }
    ++it_val;
  }
  return it_range_end;
}

bool GuardRule::VidPidValidator(const std::string &val) {
  if (val.find(':') == std::string::npos)
    return false;
  std::vector<std::string> spl;
  boost::split(spl, val, [](const char symbol) { return symbol == ':'; });
  if (spl.size() != 2)
    return false;
  if (spl[0] == "*" && spl[1] != "*")
    return false;
  for (const auto &element : spl) {
    if (element.size() > 4)
      return false;
    if (element.size() < 4) {
      return element == "*";
    }
    try {
      std::stoi(element, nullptr, 16);
    } catch (const std::exception &ex) {
      Log::Error() << "Can't parse id " << element;
      return false;
    }
  }
  return true;
}

bool GuardRule::InterfaceValidator(const std::string &val) {
  std::vector<std::string> spl;
  boost::split(spl, val, [](const char symbol) { return symbol == ':'; });
  if (spl.size() != 3)
    return false;
  if (spl[0] == "*" && (spl[1] != "*" || spl[2] != "*"))
    return false;
  if (spl[1] == "*" && spl[2] != "*")
    return false;
  for (const auto &element : spl) {
    if (element.size() != 2 && element == "*")
      return true;
    try {
      std::stoi(element, nullptr, 16);
    } catch (const std::exception &ex) {
      Log::Error() << "Can't parse interface " << val;
      return false;
    }
  }
  return true;
}

std::optional<std::pair<RuleOperator, std::vector<std::string>>>
GuardRule::ParseTokenWithOperator(
    std::vector<std::string> &splitted, const std::string &name,
    const std::function<bool(const std::string &)> &predicat) {

  std::logic_error ex_common("Cant parse rule string");
  std::optional<std::pair<RuleOperator, std::vector<std::string>>> res;
  bool have_operator{false};
  // find token
  auto it_name = std::find(splitted.cbegin(), splitted.cend(), name);
  if (it_name != splitted.cend()) {
    auto it_param = it_name;
    ++it_param;
    // if a value exists -> check may be it is an operator
    if (it_param == splitted.cend()) {
      Log::Error() << "Parsing error. No values for param " << name
                   << " found.";
      throw ex_common;
    }
    // check if first param is an operator
    auto it_operator = std::find_if(
        map_operator.cbegin(), map_operator.cend(),
        [&it_param](const std::pair<RuleOperator, std::string> &val) {
          return val.second == *it_param;
        });
    have_operator = it_operator != map_operator.cend();
    //  If no operator is found, parse as usual param - value
    if (!have_operator) {
      std::optional<std::string> tmp_value =
          ParseToken(splitted, name, predicat);
      if (tmp_value.has_value()) {
        res = {RuleOperator::no_operator, {}};
        res->second.push_back(*tmp_value);
      }
    }
    // If an operator is found,an array of values is expected.
    else {
      res = {it_operator->first, {}}; // Create a pair with an empty vector.
      auto it_range_end = ParseCurlyBracesArray(it_param, splitted.cend(),
                                                predicat, res->second);

      splitted.erase(it_name, ++it_range_end);
    }
  }
  return res;
}

/******************************************************************************/

std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>>
GuardRule::ParseConditions(std::vector<std::string> &splitted) {

  std::logic_error ex_common("Cant parse conditions");
  std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>> res;
  // Check if there are any conditions - look for "if"
  auto it_if_operator = std::find(splitted.cbegin(), splitted.cend(), "if");
  if (it_if_operator == splitted.cend())
    return std::nullopt;

  // Check if there is any operator
  bool have_operator{false};
  auto it_param1 = it_if_operator;
  ++it_param1;
  if (it_param1 == splitted.cend()) {
    Log::Error() << "No value was found for condition";
    throw ex_common;
  }
  auto it_operator = std::find_if(
      map_operator.cbegin(), map_operator.cend(),
      [&it_param1](const std::pair<RuleOperator, std::string> &val) {
        return val.second == *it_param1;
      });
  have_operator = it_operator != map_operator.cend();

  // No operator was found
  if (!have_operator) {
    std::vector<RuleWithBool> tmp;
    tmp.push_back(ParseOneCondition(it_param1, splitted.cend()));
    res = {RuleOperator::no_operator, std::move(tmp)};
    ++it_param1;
    if (it_param1 != splitted.cend()) {
      throw std::logic_error("Some text was found after a condition");
    }
    splitted.erase(it_if_operator, it_param1);
  } else {
    RuleOperator rule_operator = it_operator->first; // goes to the result

    auto range_begin = it_param1;
    ++range_begin;
    if (range_begin == splitted.cend() || *range_begin != "{") {
      throw std::logic_error("{ expected");
    }
    ++range_begin;
    if (range_begin == splitted.cend()) {
      throw ex_common;
    }
    auto range_end = std::find(range_begin, splitted.cend(), "}");
    if (range_end == splitted.cend() || range_begin >= range_end) {
      throw std::logic_error("} expected");
    }
    if (std::distance(range_begin, range_end) == 1) {
      throw std::logic_error("Empty array {} is not supported");
    }
    std::vector<RuleWithBool> tmp;
    for (auto it = range_begin; it != range_end; ++it) {
      tmp.push_back(ParseOneCondition(it, range_end));
      // it iterator will be incremented by parsing function
      // check bounds after parsing
      if (it == range_end)
        break;
    }
    ++range_end;
    if (range_end != splitted.cend()) {
      throw std::logic_error("Some text was found after conditions array");
    }
    splitted.erase(it_if_operator, range_end);
    res = {rule_operator, std::move(tmp)};
  }
  return res;
}

/******************************************************************************/

RuleWithBool GuardRule::ParseOneCondition(
    std::vector<std::string>::const_iterator &it_range_beg,
    std::vector<std::string>::const_iterator it_range_end) {

  RuleWithBool res;
  // Check for exclamation point
  bool exclamation_point = *it_range_beg == "!"; // goes to result
  if (exclamation_point) {
    ++it_range_beg;
  }
  if (it_range_beg == it_range_end)
    throw std::logic_error("Cant parse conditions");
  auto it_condition =
      std::find_if(map_conditions.cbegin(), map_conditions.cend(),
                   [&it_range_beg](const auto &pair) {
                     if (it_range_beg->size() < 2) {
                       return false;
                     } // just in case a brace trapped
                     return boost::contains(pair.second, *it_range_beg);
                   });
  if (it_condition == map_conditions.cend())
    throw std::logic_error("Cant parse this condition - " + *it_range_beg);
  RuleConditions condition = it_condition->first; // goes to result

  // Check if condition may have parameters
  bool may_have_params = CanConditionHaveParams(condition);
  bool must_have_params = MustConditionHaveParams(condition);
  // The condition, exclamation and may_have parameters are known
  ++it_range_beg;
  RuleWithOptionalParam rule_with_param{condition, std::nullopt};
  // Check if there any parameters
  if (it_range_beg != it_range_end && may_have_params) {
    std::string par_value =
        ParseConditionParameter(it_range_beg, it_range_end, must_have_params);
    // if a parameter found
    if (!par_value.empty()) {
      rule_with_param.first = ConvertToConditionWithParam(condition);
      rule_with_param.second = par_value;
      // move iterator to the end of current token
      while (it_range_beg != it_range_end && *it_range_beg != ")") {
        ++it_range_beg;
      }
    }
  } else if (it_range_beg == it_range_end && must_have_params) {
    --it_range_beg;
    throw std::logic_error("No parameters found for condition " +
                           *it_range_beg);
  }
  // move iterator back if no params were parsed
  if (!rule_with_param.second.has_value())
    --it_range_beg;
  res = {!exclamation_point, std::move(rule_with_param)};
  return res;
}

/******************************************************************************/

bool GuardRule::CanConditionHaveParams(RuleConditions cond) {
  return cond == RuleConditions::localtime ||
         cond == RuleConditions::allowed_matches ||
         cond == RuleConditions::rule_applied ||
         cond == RuleConditions::rule_applied_past ||
         cond == RuleConditions::rule_evaluated ||
         cond == RuleConditions::rule_evaluated_past ||
         cond == RuleConditions::random;
}

/******************************************************************************/

bool GuardRule::MustConditionHaveParams(RuleConditions cond) {
  return cond == RuleConditions::localtime ||
         cond == RuleConditions::allowed_matches ||
         cond == RuleConditions::rule_applied_past ||
         cond == RuleConditions::rule_evaluated_past;
}

/******************************************************************************/

std::string GuardRule::ParseConditionParameter(
    std::vector<std::string>::const_iterator it_start,
    std::vector<std::string>::const_iterator it_end, bool must_have_params) {
  std::logic_error ex_common("Can't parse parameters for condition");
  // Parse parameters.
  auto it_open_round_brace = it_start;
  if (*it_open_round_brace != "(") {
    if (must_have_params) {
      throw std::logic_error("( expected");
    }
    return "";
  }
  ++it_start;
  if (it_start == it_end) {
    throw ex_common;
  }
  auto it_close_round_brace = it_start;
  ++it_close_round_brace;
  if (it_close_round_brace == it_end || *it_close_round_brace != ")") {
    throw ex_common;
  }
  return *it_start;
}

/******************************************************************************/

RuleConditions GuardRule::ConvertToConditionWithParam(RuleConditions cond) {
  if (cond == RuleConditions::rule_applied)
    return RuleConditions::rule_applied_past;
  if (cond == RuleConditions::rule_evaluated)
    return RuleConditions::rule_evaluated_past;
  if (cond == RuleConditions::random)
    return RuleConditions::random_with_propability;
  return cond;
}

/******************************************************************************/

std::string GuardRule::ConditionsToString() const {
  std::string res;
  if (!cond)
    return "";
  // check if any operator
  if (map_operator.count(cond->first) != 0) {
    res += map_operator.at(cond->first);
    if (cond->first != RuleOperator::no_operator)
      res += "{";
  }
  int counter = 0;
  for (const auto &rule_with_bool : cond->second) {
    // look in conditions map
    if (map_conditions.count(rule_with_bool.second.first) == 0)
      continue;
    if (counter != 0)
      res += " ";
    // check for negotiation
    if (!rule_with_bool.first)
      res += '!';
    // write string representation of rule into res
    res += map_conditions.at(rule_with_bool.second.first);
    // check if any parameters exist
    if (rule_with_bool.second.second) {
      if (rule_with_bool.second.first == RuleConditions::localtime ||
          rule_with_bool.second.first == RuleConditions::allowed_matches)
        res += "(";
      res += *rule_with_bool.second.second;
      res += ")";
    }
    ++counter;
  }
  if (cond->first != RuleOperator::no_operator)
    res += "}";
  return res;
}

/******************************************************************************/

std::string
GuardRule::BuildString(bool build_parent_hash,
                       bool with_interface_array_no_operator) const noexcept {
  std::ostringstream res;
  res << map_target.at(target);
  if (vid && pid)
    res << " id " << *vid << ":" << *pid;
  if (serial)
    res << " serial " << QuoteIfNotQuoted(*serial);
  if (device_name)
    res << " name " << QuoteIfNotQuoted(*device_name);
  if (hash)
    res << " hash " << QuoteIfNotQuoted(*hash);
  if (build_parent_hash && parent_hash)
    res << " parent-hash " << QuoteIfNotQuoted(*parent_hash);
  if (port) {
    res << " via-port ";
    res << PortsToString();
  }
  if (with_interface) {
    res << " with-interface ";
    res << InterfacesToString(with_interface_array_no_operator);
  }
  if (conn_type)
    res << " with-connect-type " << QuoteIfNotQuoted(*conn_type);

  if (cond) {
    res << " if " << ConditionsToString();
  }
  std::string result = res.str();
  boost::trim(result);
  // Log::Debug() << result;
  return result;
}

/**********************************************************************************/

std::string GuardRule::PortsToString() const {
  std::stringstream string_builder;
  if (port) {
    string_builder << map_operator.at(port->first);
    if (port->first != RuleOperator::no_operator) {
      string_builder << " { ";
      for (const auto &port_val : port->second) {
        string_builder << QuoteIfNotQuoted(port_val) << " ";
      }
      string_builder << "} ";
    } else {
      string_builder << QuoteIfNotQuoted(port->second[0]);
    }
  }
  return string_builder.str();
}

/**********************************************************************************/
std::string
GuardRule::InterfacesToString(bool with_interface_array_no_operator) const {
  std::stringstream res;
  if (!with_interface.has_value())
    return res.str();
  if (with_interface->second.size() > 1) {
    if (!with_interface_array_no_operator ||
        with_interface->first != RuleOperator::equals) {
      res << map_operator.at(with_interface->first);
    }
    res << "{";
    for (const auto &interf : with_interface->second) {
      res << " " << interf;
    }
    res << " }";
  } else if (with_interface->second.size() == 1) {
    res << with_interface->second[0];
  }
  return res.str();
}

/**********************************************************************************/
vecPairs GuardRule::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("name", std::to_string(number));
  // The most string rules
  if (level == StrictnessLevel::hash) {
    res.emplace_back("lbl_rule_hash",
                     hash.has_value() ? EscapeQuotes(hash.value()) : "");
    res.emplace_back("lbl_rule_desc", device_name.has_value()
                                          ? EscapeQuotes(device_name.value())
                                          : "");
  }

  // VID::PID
  if (level == StrictnessLevel::vid_pid) {
    res.emplace_back("lbl_rule_vid", vid.has_value() ? EscapeQuotes(*vid) : "");
    res.emplace_back("lbl_rule_vendor",
                     vendor_name.has_value() ? EscapeQuotes(*vendor_name) : "");
    res.emplace_back("lbl_rule_pid", pid.has_value() ? EscapeQuotes(*pid) : "");
    res.emplace_back("lbl_rule_product",
                     device_name.has_value() ? EscapeQuotes(*device_name) : "");
    res.emplace_back("lbl_rule_port",
                     port.has_value() ? EscapeQuotes(PortsToString()) : "");
  }

  // interface
  if (level == StrictnessLevel::interface) {
    res.emplace_back(
        "lbl_rule_interface",
        with_interface.has_value() ? EscapeQuotes(InterfacesToString()) : "");
    res.emplace_back("lbl_rule_desc", device_name.has_value()
                                          ? EscapeQuotes(device_name.value())
                                          : "");
    res.emplace_back("lbl_rule_port",
                     port.has_value() ? EscapeQuotes(PortsToString()) : "");
  }

  // non-strict
  if (level == StrictnessLevel::non_strict) {
    res.emplace_back("lbl_rule_raw", EscapeQuotes(BuildString()));
  }
  res.emplace_back("lbl_rule_target",
                   map_target.count(target) != 0 ? map_target.at(target) : "");
  return res;
}

StrictnessLevel
GuardRule::StrToStrictnessLevel(const std::string &str) noexcept {
  if (str == "hash")
    return StrictnessLevel::hash;
  if (str == "vid_pid")
    return StrictnessLevel::vid_pid;
  if (str == "interface")
    return StrictnessLevel::interface;
  return StrictnessLevel::non_strict;
}

} // namespace guard