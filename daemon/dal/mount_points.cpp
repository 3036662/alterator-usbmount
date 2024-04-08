#include "mount_points.hpp"
#include "dto.hpp"
#include <memory>

namespace usbmount::dal {
Mountpoints::Mountpoints(const std::string &path) : Table(path) {
  DataFromRawJson();
}

void Mountpoints::DataFromRawJson() {
  if (raw_json_.empty())
    return;
  std::runtime_error exc("Error reading data from JSON MountPoints");
  json::value val = json::parse(raw_json_);
  if (!val.is_array())
    throw std::runtime_error("Mountpoints JSON must contain array");
  const json::array arr = val.as_array();
  for (const json::value &element : arr) {
    if (!element.is_object())
      throw exc;
    const json::object obj = element.as_object();
    if (!obj.contains("id") || !obj.at("id").is_uint64() ||
        !obj.contains("device") || !obj.at("device").is_string() ||
        !obj.contains("path") || !obj.at("path").is_string() ||
        !obj.contains("fs") || !obj.at("fs").is_string())
      throw exc;
    data_.emplace(obj.at("id").as_uint64(), std::make_shared<MountEntry>(obj));
  }
}

void Mountpoints::Create(const Dto &dto) {
  const MountEntry &entry = dynamic_cast<const MountEntry &>(dto);
  uint64_t index = data_.empty() ? 0 : (data_.rbegin()->first) + 1;
  data_.emplace(index, std::make_shared<MountEntry>(entry));
}

const MountEntry &Mountpoints::Read(uint64_t index) const {
  CheckIndex(index);
  auto entry = std::dynamic_pointer_cast<MountEntry>(data_.at(index));
  return *entry;
};

void Mountpoints::Update(uint64_t index, const Dto &dto) {
  const MountEntry &entry = dynamic_cast<const MountEntry &>(dto);
  CheckIndex(index);
  data_.at(index) = std::make_shared<MountEntry>(entry);
}

} // namespace usbmount::dal