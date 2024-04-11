#include "mount_points.hpp"
#include "dto.hpp"
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>

namespace usbmount::dal {
Mountpoints::Mountpoints(const std::string &path) : Table(path) {
  DataFromRawJson();
}

void Mountpoints::DataFromRawJson() {
  if (raw_json_.empty())
    return;

  json::value val = json::parse(raw_json_);
  if (!val.is_array())
    throw std::runtime_error("Mountpoints JSON must contain array");
  const json::array arr = val.as_array();
  std::unique_lock<std::shared_mutex> lock(data_mutex_);
  for (const json::value &element : arr) {
    if (!element.is_object())
      throw std::runtime_error("not an object");
    const json::object obj = element.as_object();
    if (!obj.contains("id") || !obj.at("id").is_number() ||
        !obj.contains("dev_name") || !obj.at("dev_name").is_string() ||
        !obj.contains("mount_point") || !obj.at("mount_point").is_string() ||
        !obj.contains("fs_type") || !obj.at("fs_type").is_string())
      throw std::runtime_error("Error reading data from JSON MountPoints");
    data_.emplace(obj.at("id").to_number<uint64_t>(),
                  std::make_shared<MountEntry>(obj));
  }
  lock.unlock();
  WriteRaw();
}

void Mountpoints::Create(const Dto &dto) {
  const MountEntry &entry = dynamic_cast<const MountEntry &>(dto);
  std::unique_lock<std::shared_mutex> lock(data_mutex_);
  uint64_t index = data_.empty() ? 0 : (data_.rbegin()->first) + 1;
  data_.emplace(index, std::make_shared<MountEntry>(entry));
  lock.unlock();
  WriteRaw();
}

const MountEntry &Mountpoints::Read(uint64_t index) const {
  CheckIndex(index);
  std::shared_lock<std::shared_mutex> lock(data_mutex_);
  auto entry = std::dynamic_pointer_cast<MountEntry>(data_.at(index));
  return *entry;
};

void Mountpoints::Update(uint64_t index, const Dto &dto) {
  const MountEntry &entry = dynamic_cast<const MountEntry &>(dto);
  CheckIndex(index);
  std::unique_lock<std::shared_mutex> lock(data_mutex_);
  data_.at(index) = std::make_shared<MountEntry>(entry);
  lock.unlock();
  WriteRaw();
}

std::optional<uint64_t>
Mountpoints::Find(const MountEntry &entry) const noexcept {
  std::shared_lock<std::shared_mutex> lock(data_mutex_);
  auto it_found = std::find_if(
      data_.cbegin(), data_.cend(),
      [&entry](const std::pair<const uint64_t, std::shared_ptr<Dto>> &element) {
        auto db_entry = std::dynamic_pointer_cast<MountEntry>(element.second);
        return *db_entry == entry;
      });
  return it_found != data_.cend() ? std::make_optional(it_found->first)
                                  : std::nullopt;
}

} // namespace usbmount::dal