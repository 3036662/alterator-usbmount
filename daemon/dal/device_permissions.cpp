/* File: device_permissions.cpp

  Copyright (C)   2024
  Author: Oleg Proskurin, <proskurinov@basealt.ru>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <https://www.gnu.org/licenses/>.

*/

#include "device_permissions.hpp"
#include "dto.hpp"
#include "table.hpp"
#include <algorithm>
// NOLINTNEXTLINE
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace usbmount::dal {

// DevicePermissions

DevicePermissions::DevicePermissions(const std::string &path) : Table(path) {
  DevicePermissions::DataFromRawJson();
}

void DevicePermissions::DataFromRawJson() {
  if (raw_json_.empty()) {
    return;
  }
  json::value val = json::parse(raw_json_);
  if (!val.is_array()) {
    throw std::runtime_error("DevicePermissions JSON must contain array");
  }
  const json::array arr = val.as_array();
  // array of tuples iteration
  std::unique_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::unique_lock(data_mutex_);
  }
  for (const json::value &element : arr) {
    if (!element.is_object()) {
      throw std::runtime_error("Invalid JSON object");
    }
    const json::object obj = element.as_object();
    if (!obj.contains("device") || !obj.at("device").is_object() ||
        !obj.contains("users") || !obj.at("users").is_array() ||
        !obj.contains("groups") || !obj.at("groups").is_array() ||
        !obj.contains("id") || obj.at("id").is_uint64()) {
      throw std::runtime_error("Ill-formed JSON object");
    }
    data_.emplace(obj.at("id").to_number<uint64_t>(),
                  std::make_shared<PermissionEntry>(obj));
  }
}

void DevicePermissions::Create(const Dto &dto) {
  const auto &entry = dynamic_cast<const PermissionEntry &>(dto);
  std::unique_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::unique_lock(data_mutex_);
  }
  uint64_t index = data_.empty() ? 0 : (data_.rbegin()->first) + 1;
  data_.emplace(index, std::make_shared<PermissionEntry>(entry));
  if (!transaction_started_) {
    lock.unlock();
    WriteRaw();
  }
}

const PermissionEntry &DevicePermissions::Read(uint64_t index) const {
  CheckIndex(index);
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::shared_lock(data_mutex_);
  }
  auto entry = std::dynamic_pointer_cast<PermissionEntry>(data_.at(index));
  if (!entry) {
    throw std::runtime_error("Pointer cast PermissionEntry failed");
  }
  return *entry;
}

std::optional<uint64_t>
DevicePermissions::Find(const Device &dev) const noexcept {
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::shared_lock(data_mutex_);
  }
  auto it_found = std::find_if(
      data_.cbegin(), data_.cend(),
      [&dev](const std::pair<const uint64_t, std::shared_ptr<Dto>> &entry) {
        return std::dynamic_pointer_cast<PermissionEntry>(entry.second)
                   ->getDevice() == dev;
      });
  return it_found != data_.cend() ? std::make_optional(it_found->first)
                                  : std::nullopt;
}

void DevicePermissions::Update(uint64_t index, const Dto &dto) {
  const auto &entry = dynamic_cast<const PermissionEntry &>(dto);
  CheckIndex(index);
  std::unique_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::unique_lock(data_mutex_);
  }
  data_.at(index) = std::make_shared<PermissionEntry>(entry);
  if (!transaction_started_) {
    lock.unlock();
    WriteRaw();
  }
}

std::map<uint64_t, std::shared_ptr<const PermissionEntry>>
DevicePermissions::getAll() const noexcept {
  std::map<uint64_t, std::shared_ptr<const PermissionEntry>> res;
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::shared_lock(data_mutex_);
  }
  for (const auto &entry : data_) {
    auto perm = std::dynamic_pointer_cast<const PermissionEntry>(entry.second);
    if (perm) {
      res.emplace(entry.first, perm);
    }
  }
  return res;
}

} // namespace usbmount::dal