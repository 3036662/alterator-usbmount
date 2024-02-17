#include "utils.hpp"
#include <iostream>

std::string ToLisp([[maybe_unused]] const std::string &name,
                   const std::string &value) {
  std::string res;
  // ignore name - use olny value
  res += "(";
  res += WrapWithQuotes(value);
  res += ")";
  return res;
}

std::string EscapeQuotes(const std::string &str) {
  std::string res;
  for (auto it = str.cbegin(); it != str.cend(); ++it) {
    if (*it == '\"' && res.back() != '\\') {
      res.push_back('\\');
    }
    res.push_back(*it);
  }
  return res;
}

std::string WrapWithQuotes(const std::string &str) {
  std::string res;
  res += "\"";
  res += str;
  res += "\"";
  return res;
}

std::string QuoteIfNotQuoted(const std::string &str) {
  if (str.empty())
    return "\"\"";
  if (str[0] != '\"' || str.back() != '\"')
    return WrapWithQuotes(str);
  return str;
}

std::vector<guard::UsbDevice> fakeLibGetUsbList() {
  std::vector<guard::UsbDevice> res;
  for (int i = 0; i < 10; ++i) {
    std::string str_num = std::to_string(i);
    res.emplace_back(i, "allowed", "name" + str_num, "vid" + str_num,
                     "pid" + str_num, "port" + str_num, "conn" + str_num,
                     "00::00::00", "serial" + str_num, "hash" + str_num);
  }
  return res;
}

uint32_t StrToUint(const std::string &str) noexcept {
  uint32_t res = 0;
  try {
    res = static_cast<uint32_t>(std::stoul(str));
  } catch (std::exception &e) {
    std::cerr << "Error string to number conversion";
    std::cerr << e.what();
  }
  return res;
}

std::vector<std::string> FindAllFilesInDirRecursive(const std::string &dir,
                                                    const std::string &ext) {
  // TODO think about enabling symlinks support
  namespace fs = std::filesystem;
  const int max_depth = 30;
  std::vector<std::string> res;
  fs::path fs_path(dir);
  if (fs::exists(fs_path)) {
    for (auto it_entry = fs::recursive_directory_iterator(fs_path);
         it_entry != fs::recursive_directory_iterator(); ++it_entry) {
      if (it_entry.depth() > max_depth)
        break;
      if (it_entry->is_regular_file() &&
          it_entry->path().extension().string() == ext) {
        res.emplace_back(it_entry->path().string());
      }
    }
  }
  return res;
}

std::vector<uint> ParseJsonIntArray(std::string json) noexcept {
  std::vector<uint> res;
  boost::trim(json);
  if (json.size() < 3)
    return res;
  if (json[0] != '[' || json.back() != ']')
    return res;
  // remove [] braces
  json.erase(json.size() - 1, 1);
  json.erase(0, 1);
  std::vector<std::string> splitted;
  boost::split(splitted, json, [](const char c) { return c == ','; });
  if (splitted.empty())
    return res;
  // convert to ints and push to vector
  std::for_each(splitted.begin(), splitted.end(), [&res](std::string &el) {
    boost::trim(el);
    if (el.empty())
      return;
    if (el.back() == '"')
      el.erase(el.size() - 1, 1);
    if (!el.empty() && el[0] == '"')
      el.erase(0, 1);
    try {
      uint x = std::stoi(el);
      res.push_back(x);
    } catch (const std::exception &ex) {
      std::cerr << "[ERROR] Can't parse int rules number from json " << el
                << std::endl;
    }
  });
  return res;
};