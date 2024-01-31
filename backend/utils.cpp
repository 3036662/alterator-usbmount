#include "utils.hpp"
#include <iostream>

std::string ToLisp(const std::string& name,const std::string& value){
  std::string res;
  // ignore name - use olny value
  res+="(";
  res+=WrapWithQuotes(value);
  res+=")";
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