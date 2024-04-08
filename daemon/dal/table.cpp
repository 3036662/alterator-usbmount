#include "table.hpp"
#include "dto.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <sys/types.h>

namespace usbmount::dal {

namespace fs = std::filesystem;

// CRUD Table
Table::Table(const std::string &data_file_path) : file_path_(data_file_path) {
  if (!fs::exists(file_path_)) {
    fs::create_directories(fs::path(file_path_).parent_path());
    std::ofstream file(file_path_, std::ios_base::out);
    if (!file.is_open())
      throw std::runtime_error("Can't open " + data_file_path);
    file.close();
  } else {
    ReadRaw();
  }
}

void Table::ReadRaw() {
  if (!fs::exists(file_path_))
    return;
  std::ifstream file(file_path_);
  if (!file.is_open())
    throw std::runtime_error("Can't open " + file_path_);
  raw_json_ = std::string(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>());
  file.close();
}

json::value Table::ToJson() const noexcept {
  json::array res;
  for (const auto &entry : data_) {
    json::object js_entry = entry.second->ToJson().as_object();
    js_entry["id"] = entry.first;
    res.emplace_back(std::move(js_entry));
  }
  return res;
}

void Table::CheckIndex(uint64_t index) const {
  if (data_.count(index) == 0)
    throw std::invalid_argument(kWrongArg);
}

void Table::Delete(uint64_t index) noexcept { data_.erase(index); }

uint64_t Table::size() const noexcept { return data_.size(); }

void Table::Clear() noexcept { data_.clear(); }

} // namespace usbmount::dal
