#include "guard_rule.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <functional>
#include <map>

namespace guard {

GuardRule::GuardRule(std::string raw_str) {
  std::logic_error ex("Cant parse rule string");
  // find all quoted substrings
  std::vector<std::string> splitted{SplitRawRule(raw_str)};
  if (splitted.empty())
    throw ex;

  // map strings to values
  // target is mandatory
  auto it_target = std::find_if(
      map_target.cbegin(), map_target.cend(),
      [&splitted](const auto &el) { return el.second == splitted[0]; });
  if (it_target == map_target.cend()) {
    throw ex;
  }
  target = it_target->first;
  if (splitted.size() == 1)
    return;

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

  hash = ParseToken(splitted, "hash", [this](const std::string &val) {
    return val.size() == usbguard_hash_length;
  });

  device_name = ParseToken(splitted, "name", [this](const std::string &val) {
    return !IsReservedWord(val);
  });

  serial = ParseToken(splitted, "serial", [this](const std::string &val) {
    return !IsReservedWord(val);
  });

  port = ParseTokenWithOperator(
      splitted, "via-port",
      [this](const std::string &val) { return !IsReservedWord(val); });
}

std::vector<std::string> GuardRule::SplitRawRule(std::string raw_str) {
  boost::trim(raw_str);
  std::vector<std::string> res;
  if (raw_str.empty())
    return res;
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
  if (it_fast == raw_str.end() && !dont_split && it_slow < it_fast) {
    res.emplace_back(it_slow, it_fast);
  }
  for (std::string &s : res) {
    boost::trim(s);
  }
  for (auto it = res.begin(); it != res.end(); ++it) {
    if (it->empty())
      res.erase(it);
  }
  return res;
}

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
  return false;
}

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
      std::cerr << "[ERROR] Parsing hash error" << std::endl;
      throw ex;
    }
  }

  return res;
}

std::optional<std::pair<RuleOperator, std::vector<std::string>>>
GuardRule::ParseTokenWithOperator(
    const std::vector<std::string> &splitted, const std::string &name,
    std::function<bool(const std::string &)> predicat) const {
  std::logic_error ex("Cant parse rule string");
  std::optional<std::pair<RuleOperator, std::vector<std::string>>> res;
  // find out if there is any operator
  bool have_operator{false};
  auto it_name = std::find(splitted.cbegin(), splitted.cend(), name);
  if (it_name != splitted.cend()) {
    auto it_name_param1 = std::move(++it_name);
    // if a value exists -> check may be it is an operator
    if (it_name_param1 != splitted.cend()) {
      std::cerr << "it_name_param1" << *it_name_param1 << std::endl;
      // check if first param is an operator
      auto it_operator = std::find_if(
          map_operator.cbegin(), map_operator.cend(),
          [&it_name_param1](const std::pair<RuleOperator, std::string> &val) {
            return val.second == *it_name_param1;
          });

      have_operator = it_operator != map_operator.cend();
      std::cerr << "have_operator " << have_operator << std::endl;
      // If no operator is found, parse as usual param - value
      if (!have_operator) {
        std::optional<std::string> tmp_value =
            ParseToken(splitted, name, predicat);
        std::cerr << "tmp_value has value" << tmp_value.has_value()
                  << std::endl;
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
    // No values found for this param
    else {
      std::cerr << "[ERROR] Parsing hash error. No values for param " << name
                << " found." << std::endl;
      throw ex;
    }
  }
  return res;
}

} // namespace guard

/*
allow
id 1d6b:0003
serial "0000:00:0d.0"
name "xHCI Host Controller"
hash "4Q3Ski/Lqi8RbTFr10zFlIpagY9AKVMszyzBQJVKE+c="
parent-hash "Y1kBdG1uWQr5CjULQs7uh2F6pHgFb6VDHcWLk83v+tE="
with-interface 09:00:00 with-connect-type ""
*/