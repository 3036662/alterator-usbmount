#include "device_permissions.hpp"
#include "dto.hpp"
#include <memory>

namespace usbmount::dal {

// DevicePermissions

DevicePermissions::DevicePermissions(const std::string &path) : Table(path) {
  DataFromRawJson();
}

void DevicePermissions::DataFromRawJson() {
  if (raw_json_.empty())
    return;
  std::runtime_error exc("Error reading data from JSON DevicePermissions");
  json::value val = json::parse(raw_json_);
  if (!val.is_array())
    throw std::runtime_error("DevicePermissions JSON must contain array");
  const json::array arr = val.as_array();
  // array of tuples iteration
  for (const json::value &element : arr) {
    if (!element.is_object())
      throw exc;
    const json::object obj = element.as_object();
    if (!obj.contains("device") || !obj.at("device").is_string() ||
        !obj.contains("users") || !obj.at("users").is_array() ||
        !obj.contains("groups") || !obj.at("groups").is_array() ||
        !obj.contains("id") || obj.at("id").is_uint64())
      throw exc;
    data_.emplace(std::make_pair(obj.at("id").as_uint64(),
                                 std::make_shared<PermissionEntry>(obj)));
  }
}

// json::value DevicePermissions::ToJson() const noexcept {
//   json::array res;
//   for (const auto &entry : data_) {
//     json::object js_entry = entry.second.ToJson().as_object();
//     js_entry["id"] = entry.first;
//     res.emplace_back(std::move(js_entry));
//   }
//   return res;
// }

void DevicePermissions::Create(const Dto &dto) {
  const PermissionEntry &entry = dynamic_cast<const PermissionEntry &>(dto);
  uint64_t index = data_.empty() ? 0 : (data_.rbegin()->first) + 1;
  data_.emplace(index, std::make_shared<PermissionEntry>(entry));
}

const PermissionEntry &DevicePermissions::Read(uint64_t index) const {
  CheckIndex(index);
  auto entry = std::dynamic_pointer_cast<PermissionEntry>(data_.at(index));
  return *entry;
};

void DevicePermissions::Update(uint64_t index, const Dto &dto) {
  const PermissionEntry &entry = dynamic_cast<const PermissionEntry &>(dto);
  CheckIndex(index);
  data_.at(index) = std::make_shared<PermissionEntry>(entry);
}

} // namespace usbmount::dal