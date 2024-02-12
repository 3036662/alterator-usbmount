#include "guard_rule.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <functional>
#include <map>

namespace guard {

/******************************************************************************/

GuardRule::GuardRule(const std::string &raw_str)
    : map_target{{Target::allow, "allow"},
                 {Target::block, "block"},
                 {Target::reject, "reject"}},
      map_conditions{{RuleConditions::localtime, "localtime"},
                     {RuleConditions::allowed_matches, "allowed-matches"},
                     {RuleConditions::rule_applied, "rule-applied"},
                     {RuleConditions::rule_applied_past, "rule-applied("},
                     {RuleConditions::rule_evaluated, "rule-evaluated"},
                     {RuleConditions::rule_evaluated_past, "rule-evaluated("},
                     {RuleConditions::random, "random"},
                     {RuleConditions::random_with_propability, "random("},
                     {RuleConditions::always_true, "true"},
                     {RuleConditions::always_false, "false"},
                     {RuleConditions::no_condition, ""}},
      map_operator{{RuleOperator::all_of, "all-of"},
                   {RuleOperator::one_of, "one-of"},
                   {RuleOperator::none_of, "none-of"},
                   {RuleOperator::equals, "equals"},
                   {RuleOperator::equals_ordered, "equals-ordered"},
                   {RuleOperator::no_operator, ""}} {

  std::logic_error ex("Cant parse rule string");
  // Split string to tokens.
  std::vector<std::string> splitted{SplitRawRule(raw_str)};
  if (splitted.empty())
    throw ex;

  // Map strings to values.
  // ----------------------------------------
  // The target is mandatory.
  auto it_target = std::find_if(
      map_target.cbegin(), map_target.cend(),
      [&splitted](const auto &el) { return el.second == splitted[0]; });
  if (it_target == map_target.cend()) {
    throw ex;
  }
  target = it_target->first;
  if (splitted.size() == 1)
    return;

  // ----------------------------------------
  // id
  std::optional<std::string> str_id =
      ParseToken(splitted, "id", [](const std::string &val) {
        if (val.find(':') == std::string::npos)
          return false;
        return true;
      });
  if (str_id) {
    size_t separator = splitted[2].find(':');
    vid = splitted[2].substr(0, separator);
    pid = splitted[2].substr(separator + 1);
    if (vid->empty() || pid->empty())
      throw ex;
  }

  // ----------------------------------------
  // Hash, device_name, serial, port, and with_interface may or may not exist in
  // the string. default_predicat - Function is the default behavior of values
  // validating.
  std::function<bool(const std::string &)> default_predicat =
      [this](const std::string &val) { return !IsReservedWord(val); };
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
  with_interface = ParseTokenWithOperator(
      splitted, "with-interface", [](const std::string &val) {
        return val.size() > 2 && val.find(':') != std::string::npos;
      });

  conn_type = ParseToken(splitted, "with-connect-type", default_predicat);
  // Conditions may or may not exist in the string.
  cond = ParseConditions(splitted);
}

/******************************************************************************/

std::vector<std::string> GuardRule::SplitRawRule(std::string raw_str) {
  boost::trim(raw_str);
  std::vector<std::string> res;
  if (raw_str.empty())
    return res;
  // Wrap all curly and round braces and exclamation points with spaces
  {
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
  for (std::string &s : res)
    boost::trim(s);
  for (auto it = res.begin(); it != res.end(); ++it) {
    if (it->empty())
      res.erase(it);
  }
  return res;
}

/******************************************************************************/

bool GuardRule::IsReservedWord(const std::string &str) {
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

std::optional<std::string>
GuardRule::ParseToken(const std::vector<std::string> &splitted,
                      const std::string &name,
                      std::function<bool(const std::string &)> predicat) const {
  std::logic_error ex("Cant parse rule string");
  std::optional<std::string> res;
  auto it_name = std::find(splitted.cbegin(), splitted.cend(), name);
  if (it_name != splitted.cend()) {
    auto it_name_param = std::move(++it_name);
    if (it_name_param != splitted.cend() && predicat(*it_name_param)) {
      res = *it_name_param;
    } else {
      std::cerr << "[ERROR] Parsing error" << std::endl;
      throw ex;
    }
  }

  return res;
}

/******************************************************************************/

std::optional<std::pair<RuleOperator, std::vector<std::string>>>
GuardRule::ParseTokenWithOperator(
    const std::vector<std::string> &splitted, const std::string &name,
    std::function<bool(const std::string &)> predicat) const {
  std::logic_error ex("Cant parse rule string");
  std::optional<std::pair<RuleOperator, std::vector<std::string>>> res;

  bool have_operator{false};
  auto it_name = std::find(splitted.cbegin(), splitted.cend(), name);
  if (it_name != splitted.cend()) {
    auto it_name_param1 = std::move(++it_name);
    // if a value exists -> check may be it is an operator
    if (it_name_param1 == splitted.cend()) {
      std::cerr << "[ERROR] Parsing error. No values for param " << name
                << " found." << std::endl;
      throw ex;
    }
    // std::cerr << "it_name_param1 " << *it_name_param1 << std::endl;
    //  check if first param is an operator
    auto it_operator = std::find_if(
        map_operator.cbegin(), map_operator.cend(),
        [&it_name_param1](const std::pair<RuleOperator, std::string> &val) {
          return val.second == *it_name_param1;
        });

    have_operator = it_operator != map_operator.cend();
    //  If no operator is found, parse as usual param - value
    if (!have_operator) {
      std::optional<std::string> tmp_value =
          ParseToken(splitted, name, predicat);
      if (tmp_value) {
        res = {RuleOperator::no_operator, {}};
        res->second.push_back(*tmp_value);
      }
    }
    // If an operator is found,an array of values is expected.
    else {
      res = {it_operator->first, {}}; // Create a pair with an empty vector.
      auto it_range_begin = it_name_param1;
      ++it_range_begin;
      // if no array found -> throw exception
      if (it_range_begin == splitted.cend() || *it_range_begin != "{") {
        std::cerr << "[ERROR] Error parsing values for " << name << " param."
                  << std::endl;
        throw ex;
      }
      auto it_range_end = std::find(it_range_begin, splitted.cend(), "}");
      if (it_range_end == splitted.cend()) {
        std::cerr << "[ERROR] A closing \"}\" expectend for sequence."
                  << std::endl;
        throw ex;
      }
      // fill result with values
      auto it_val = it_range_begin;
      ++it_val;
      while (it_val != it_range_end) {
        res->second.emplace_back(*it_val);
        ++it_val;
      }
    }
  }
  return res;
}

/******************************************************************************/

std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>>
GuardRule::ParseConditions(const std::vector<std::string> &splitted) {
  std::logic_error ex("Cant parse conditions");
  std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>> res;
  // ------------------------------------------------------
  // Check if there are any conditions - look for "if"
  auto it_if_operator = std::find(splitted.cbegin(), splitted.cend(), "if");
  if (it_if_operator == splitted.cend())
    return std::nullopt;

  // Check if there is any operator
  bool have_operator{false};
  auto it_param1 = std::move(++it_if_operator);
  if (it_param1 == splitted.cend()) {
    std::cerr << "[ERROR] No value was found for condition" << std::endl;
    throw ex;
  }
  auto it_operator = std::find_if(
      map_operator.cbegin(), map_operator.cend(),
      [&it_param1](const std::pair<RuleOperator, std::string> &val) {
        return val.second == *it_param1;
      });
  have_operator = it_operator != map_operator.cend();
  std::cerr << "Has operator " << have_operator << std::endl;

  // ------------------------------------------------------
  // No operator was found
  if (!have_operator) {
    std::vector<RuleWithBool> tmp;
    tmp.push_back(ParseOneCondition(it_param1, splitted.cend()));
    res = {RuleOperator::no_operator, std::move(tmp)};
  } else {
    std::cerr << "Operator was found =" << it_operator->second << std::endl;
    // Find the array bounds.
    RuleOperator op = it_operator->first; // goes to the result

    auto range_begin = it_param1;
    ++range_begin;
    if (range_begin == splitted.cend() || *range_begin != "{") {
      throw ex;
    }
    ++range_begin;
    if (range_begin == splitted.cend()) {
      throw ex;
    }
    auto range_end = std::find(range_begin, splitted.cend(), "}");
    if (range_end == splitted.cend() || range_begin >= range_end) {
      throw ex;
    }
    std::vector<RuleWithBool> tmp;
    for (auto it = range_begin; it != range_end; ++it) {
      tmp.push_back(ParseOneCondition(it, range_end));
      /* it iterator will be incremented by parsing function
       * checlk bounds after parsing
       */
      if (it == range_end)
        break;
    }
    res = {op, std::move(tmp)};
  }
  return res;
}

/******************************************************************************/

RuleWithBool GuardRule::ParseOneCondition(
    std::vector<std::string>::const_iterator &it_range_beg,
    std::vector<std::string>::const_iterator it_range_end) const {

  RuleWithBool res;
  std::logic_error ex("Cant parse conditions");
  // Check for exclamation point
  bool exclamation_point = *it_range_beg == "!"; // goes to result
  if (exclamation_point) {
    ++it_range_beg;
  }
  if (it_range_beg == it_range_end)
    throw ex;
  auto it_condition =
      std::find_if(map_conditions.cbegin(), map_conditions.cend(),
                   [&it_range_beg](const auto &pair) {
                     if (it_range_beg->size() < 2) {
                       return false;
                     } // just in case a brace trapped
                     return boost::contains(pair.second, *it_range_beg);
                   });
  if (it_condition == map_conditions.cend())
    throw ex;
  RuleConditions condition = it_condition->first; // goes to result

  // Check if condition may have parameters
  bool may_have_params = CanConditionHaveParams(condition);
  // The condition, exclamation and may_have parameters are known
  // Check if there any parameters
  ++it_range_beg;
  // some params found
  if (it_range_beg != it_range_end) {
    /* Conditions are the last block of rule string
     * So, if a condition can not have any parameters, existance of any
     * parameters means an error. */
    if (!may_have_params) {
      std::cerr << "[ERROR] Condition "
                << map_conditions.find(condition)->second
                << " can't have parameters." << std::endl;
      throw ex;
    }
    std::string par_value = ParseConditionParameter(it_range_beg, it_range_end);
    condition = ConvertToConditionWithParam(condition);
    RuleWithOptionalParam rule_with_param{condition, par_value};
    res = {!exclamation_point, rule_with_param};
    // move iterator to the end of current token
    while (it_range_beg != it_range_end && *it_range_beg != ")") {
      ++it_range_beg;
    }
  }
  // no parameters found
  else {
    RuleWithOptionalParam rule_with_param{condition, std::nullopt};
    res = {!exclamation_point, rule_with_param};
  }
  return res;
}

/******************************************************************************/

bool GuardRule::CanConditionHaveParams(RuleConditions cond) const {
  return cond == RuleConditions::localtime ||
         cond == RuleConditions::allowed_matches ||
         cond == RuleConditions::rule_applied ||
         cond == RuleConditions::rule_applied_past ||
         cond == RuleConditions::rule_evaluated ||
         cond == RuleConditions::rule_evaluated_past ||
         cond == RuleConditions::random;
}

/******************************************************************************/

std::string GuardRule::ParseConditionParameter(
    std::vector<std::string>::const_iterator it_start,
    std::vector<std::string>::const_iterator it_end) const {
  std::logic_error ex("Can't parse parameters for condition");
  // Parse parameters.
  auto it_open_round_brace = it_start;
  if (*it_open_round_brace != "(") {
    throw ex;
  }
  ++it_start;
  if (it_start == it_end) {
    throw ex;
  }
  auto it_close_round_brace = it_start;
  ++it_close_round_brace;
  if (it_close_round_brace == it_end || *it_close_round_brace != ")") {
    throw ex;
  }
  return *it_start;
}

/******************************************************************************/

RuleConditions
GuardRule::ConvertToConditionWithParam(RuleConditions cond) const {
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
    return "no conditions \n";
  if (map_operator.count(cond->first)) {
    res += map_operator.at(cond->first);
    if (cond->first != RuleOperator::no_operator)
      res += "{";
  }
  int counter = 0;
  for (const auto &rule_with_bool : cond->second) {
    if (!map_conditions.count(rule_with_bool.second.first))
      continue;
    if (counter)
      res += " ";
    if (!rule_with_bool.first)
      res += '!';
    res += map_conditions.at(rule_with_bool.second.first);
    if (rule_with_bool.second.second) {
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
                       bool with_interface_array_no_operator) const {
  std::string res;
  res += map_target.at(target);
  if (vid && pid) {
    res += " id ";
    res += *vid;
    res += ":";
    res += *pid;
  }
  if (serial) {
    res += " serial ";
    res += *serial;
  }
  if (device_name) {
    res += " name ";
    res += *device_name;
  }
  if (hash) {
    res += " hash ";
    res += *hash;
  }
  if (build_parent_hash && parent_hash) {
    res += " parent-hash ";
    res += *parent_hash;
  }

  if (port) {
    res += " via-port ";
    res += map_operator.at(port->first);
    if (port->second.size() > 1) {
      res += "{ ";
    }
    for (const auto &p : port->second) {
      res += p;
    }
    if (port->second.size() > 1) {
      res += " }";
    }
  }

  if (with_interface) {
    res += " with-interface ";
    if (with_interface->second.size() > 1) {
      if (!with_interface_array_no_operator ||
          with_interface->first != RuleOperator::equals) {
        res += map_operator.at(with_interface->first);
      }
      res += "{";
      for (const auto &i : with_interface->second) {
        res += " ";
        res += i;
      }
      res += " }";
    } else if (with_interface->second.size() == 1) {
      res += with_interface->second[0];
    }
  }
  if (conn_type) {
    res += " with-connect-type ";
    res += *conn_type;
  }

  if (cond) {
    res += " ";
    res += ConditionsToString();
  }
  boost::trim(res);
  return res;
}

} // namespace guard