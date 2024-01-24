#include "utils.hpp"
#include <iostream>

// convert vector of string pairs to lisp list ("wierd_val_called_name" "name"
// "value" "name2" "value2")
std::string ToLisp(const vecPairs &vec) {
  std::string res;
  res += "(";
  // name for list - alterator expectes first value to be
  // something called "name" -> just put vec[0].second
  // TODO find out if it used somewhere

  for (const auto &pair : vec) {
    res += WrapWithQuotes(pair.second);
    res += " ";
    res += WrapWithQuotes(pair.first);
    res += " ";
  }
  // res+=WrapWithQuotes(vec[0].second);
  res += WrapWithQuotes("");
  res += " ";
  res += ")";
  // std::cerr << "result string: " <<std::endl <<res << std::endl;
  return res;
}

std::string WrapWithQuotes(const std::string &str) {
  std::string res;
  res += "\"";
  res += str;
  res += "\"";
  return res;
}

std::vector<UsbDevice> fakeLibGetUsbList() {
  std::vector<UsbDevice> res;
  for (int i = 0; i < 10; ++i) {
    std::string str_num = std::to_string(i);
    res.emplace_back(i, "allowed", "name" + str_num, "id" + str_num,
                     "port" + str_num, "conn" + str_num);
  }
  return res;
}

uint32_t StrToUint(const std::string &id) noexcept {
  uint32_t res = 0;
  try {
    res = static_cast<uint32_t>(std::stoul(id));
  } catch (std::exception e) {
    std::cerr << "Error string to number conversion";
    std::cerr << e.what();
  }
  return res;
}