#include "utils.hpp"
#include "log.hpp"
#include "usb_device.hpp"
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace utils {

std::string UnUtf8(const std::string &str) noexcept {
  std::string res;
  size_t ind = 0;
  std::runtime_error ex_bad_string("Invalid characters in string");
  try {
    while (ind < str.size()) {
      // the "" - to sympol quotes - skip one
      if ((str.at(ind) & 0b10000000) == 0 && ind < str.size() - 1 &&
          str.at(ind) == char(0x22) && str.at(ind + 1) == char(0x22)) {
        // if this symbol is " and next symbol is " skip this
        ++ind;
        continue;
      }
      // 1 byte char
      if ((str.at(ind) & 0b10000000) == 0) {
        res += str[ind]; // just push to result
        ++ind;
        continue;
      }
      // 2 bytes quotes
      if ((str.at(ind) & 0b11100000) == 0b11000000 && ind < str.size() - 1) {
        // \u00BB \u00AB
        if (str.at(ind) == char(0xC2) &&
            (str.at(ind + 1) == char(0xBB) || str.at(ind + 1) == char(0xAB))) {
          res += '\"';
          ind += 2;
          continue;
        }
      }
      // 3 bytes quotes
      if ((str.at(ind) & 0b11110000) == 0b11100000 && ind < str.size() - 2) {
        // \u2018 \u2019  \u201c \u201d
        if (str.at(ind) == char(0xE2) && str.at(ind + 1) == char(0x80) &&
            (str.at(ind + 2) == char(0x98) || str.at(ind + 2) == char(0x99) ||
             str.at(ind + 2) == char(0x9D) || str.at(ind + 2) == char(0x9C0) ||
             str.at(ind + 2) == char(0x9E))) {
          res += '\"';
          ind += 3;
          continue;
        }
      }
      // 4 bytes just skip
      if ((str.at(ind) & 0b11111000) == 0b11110000) {
        ind += 4;
        continue;
      }
      throw std::runtime_error("Bad utf-8 string");
    }
  } catch (const std::exception &ex) {
    guard::utils::Log::Error() << ex.what();
  }
  return res;
}

std::string WrapWithQuotes(const std::string &str) noexcept {
  std::string res;
  res += "\"";
  res += str;
  res += "\"";
  return res;
}

std::string UnQuote(const std::string &str) noexcept {
  std::string res = str;
  try {
    if (str.size() >= 2 && str[0] == '\"' && str.back() == '\"') {
      res = std::string(str, 1, str.size() - 2);
    }
  } catch (const std::exception &ex) {
    guard::utils::Log::Debug() << "Error Unquoting string(UnQoute)";
    return str;
  }
  return res;
}

std::string QuoteIfNotQuoted(const std::string &str) noexcept {
  if (str.empty())
    return "\"\"";
  if (str[0] != '\"' || str.back() != '\"')
    return WrapWithQuotes(str);
  return str;
}

std::optional<uint32_t> StrToUint(const std::string &str) noexcept {
  uint32_t res = 0;
  try {
    res = static_cast<uint32_t>(std::stoul(str));
  } catch (std::exception &e) {
    std::cerr << "Error string to number conversion";
    std::cerr << e.what();
  }
  return res;
}

std::vector<std::string> FindAllFilesInDirRecursive(
    const std::pair<std::string, std::string> &path_ext) noexcept {
  // TODO think about enabling symlinks support
  namespace fs = std::filesystem;
  const int max_depth = 30;
  std::vector<std::string> res;
  fs::path fs_path(path_ext.first);
  if (fs::exists(fs_path)) {
    for (auto it_entry = fs::recursive_directory_iterator(fs_path);
         it_entry != fs::recursive_directory_iterator(); ++it_entry) {
      if (it_entry.depth() > max_depth)
        break;
      if (it_entry->is_regular_file() &&
          it_entry->path().extension().string() == path_ext.second) {
        res.emplace_back(it_entry->path().string());
      }
    }
  }
  return res;
}

std::string ToLisp(const std::pair<std::string, std::string> &data) noexcept {
  std::string res;
  // ignore name - use olny value
  res += "(";
  res += WrapWithQuotes(data.second);
  res += ")";
  return res;
}

std::string EscapeQuotes(const std::string &str) noexcept {
  std::string res;
  for (auto it = str.cbegin(); it != str.cend(); ++it) {
    if (*it == '\"' && res.back() != '\\') {
      res.push_back('\\');
    }
    res.push_back(*it);
  }
  return res;
}

std::string EscapeAll(const std::string &str) noexcept {
  std::string res;
  for (auto it = str.cbegin(); it != str.cend(); ++it) {
    if (*it == '\"' || *it == '\\') {
      res.push_back('\\');
    }
    res.push_back(*it);
  }
  return res;
}

std::vector<guard::UsbDevice> fakeLibGetUsbList() noexcept {
  std::vector<guard::UsbDevice> res;
  for (uint i = 0; i < 10; ++i) {
    std::string str_num = std::to_string(i);
    guard::UsbDevice::DeviceData dev_data{i,
                                          "allowed",
                                          "name" + str_num,
                                          "vid" + str_num,
                                          "pid" + str_num,
                                          "port" + str_num,
                                          "conn" + str_num,
                                          "00::00::00",
                                          "serial" + str_num,
                                          "hash" + str_num};
    res.emplace_back(dev_data);
  }
  return res;
}

} // namespace utils