#include "utils.hpp"
#include "usb_device.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

namespace utils {

std::string WrapWithQuotes(const std::string &str) noexcept {
  std::string res;
  res += "\"";
  res += str;
  res += "\"";
  return res;
}

std::string UnQuote(const std::string &str) noexcept {
  std::string res = str;
  if (str.size() >= 2 && str[0] == '\"' && str.back() == '\"') {
    res = str;
    res.erase(res.front());
    res.erase(res.back());
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