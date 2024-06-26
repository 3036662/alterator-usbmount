/* File: mount_points.cpp

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

#include "mount_points.hpp"
#include "dto.hpp"
#include "table.hpp"
#include <algorithm>
// NOLINTNEXTLINE
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace usbmount::dal {
Mountpoints::Mountpoints(const std::string &path) : Table(path) {
  Mountpoints::DataFromRawJson();
}

void Mountpoints::DataFromRawJson() {
  if (raw_json_.empty()) {
    return;
  }
  json::value val = json::parse(raw_json_);
  if (!val.is_array()) {
    throw std::runtime_error("Mountpoints JSON must contain array");
  }
  const json::array arr = val.as_array();
  std::unique_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::unique_lock(data_mutex_);
  }
  for (const json::value &element : arr) {
    if (!element.is_object()) {
      throw std::runtime_error("not an object");
    }
    const json::object obj = element.as_object();
    if (!obj.contains("id") || !obj.at("id").is_number() ||
        !obj.contains("dev_name") || !obj.at("dev_name").is_string() ||
        !obj.contains("mount_point") || !obj.at("mount_point").is_string() ||
        !obj.contains("fs_type") || !obj.at("fs_type").is_string()) {
      throw std::runtime_error("Error reading data from JSON MountPoints");
    }
    data_.emplace(obj.at("id").to_number<uint64_t>(),
                  std::make_shared<MountEntry>(obj));
  }
  if (!transaction_started_) {
    lock.unlock();
  }
}

void Mountpoints::Create(const Dto &dto) {
  const auto &entry = dynamic_cast<const MountEntry &>(dto);
  // check an index to guarantee there is no such entry in the database.
  auto index = Find(entry);
  if (!index) {
    std::unique_lock<std::shared_mutex> lock;
    if (!transaction_started_) {
      lock = std::unique_lock(data_mutex_);
    }
    index = data_.empty() ? 0 : (data_.rbegin()->first) + 1;
    if (index) {
      data_.emplace(*index, std::make_shared<MountEntry>(entry));
    }
    if (!transaction_started_) {
      lock.unlock();
      WriteRaw();
    }
  }
}

const MountEntry &Mountpoints::Read(uint64_t index) const {
  CheckIndex(index);
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::shared_lock(data_mutex_);
  }
  auto entry = std::dynamic_pointer_cast<MountEntry>(data_.at(index));
  if (!entry) {
    throw std::runtime_error("Pointer cast to MountEntry failed");
  }
  return *entry;
}

void Mountpoints::Update(uint64_t index, const Dto &dto) {
  const auto &entry = dynamic_cast<const MountEntry &>(dto);
  CheckIndex(index);
  std::unique_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::unique_lock(data_mutex_);
  }
  data_.at(index) = std::make_shared<MountEntry>(entry);
  if (!transaction_started_) {
    lock.unlock();
    WriteRaw();
  }
}

std::optional<uint64_t>
Mountpoints::Find(const MountEntry &entry) const noexcept {
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::shared_lock(data_mutex_);
  }
  auto it_found = std::find_if(
      data_.cbegin(), data_.cend(),
      [&entry](const std::pair<const uint64_t, std::shared_ptr<Dto>> &element) {
        auto db_entry = std::dynamic_pointer_cast<MountEntry>(element.second);
        if (!db_entry) {
          return false;
        }
        return *db_entry == entry;
      });
  return it_found != data_.cend() ? std::make_optional(it_found->first)
                                  : std::nullopt;
}

std::optional<uint64_t>
Mountpoints::Find(const std::string &block_dev) const noexcept {
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::shared_lock(data_mutex_);
  }
  auto it_found = std::find_if(
      data_.cbegin(), data_.cend(),
      [&block_dev](
          const std::pair<const uint64_t, std::shared_ptr<Dto>> &element) {
        auto db_entry = std::dynamic_pointer_cast<MountEntry>(element.second);
        if (!db_entry) {
          return false;
        }
        return db_entry->dev_name() == block_dev;
      });
  return it_found != data_.cend() ? std::make_optional(it_found->first)
                                  : std::nullopt;
}

void Mountpoints::RemoveExpired(
    const std::unordered_set<std::string> &valid_set) noexcept {
  std::unique_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::unique_lock(data_mutex_);
  }
  for (auto it = data_.cbegin(); it != data_.cend();) {
    auto mnt_entry = std::dynamic_pointer_cast<MountEntry>(it->second);
    if (mnt_entry && valid_set.count(mnt_entry->mount_point()) == 0) {
      it = data_.erase(it);
    } else {
      ++it;
    }
  }
  if (!transaction_started_) {
    lock.unlock();
    WriteRaw();
  }
}

std::vector<MountEntry> Mountpoints::GetAll() const noexcept {
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_) {
    lock = std::shared_lock(data_mutex_);
  }
  std::vector<MountEntry> res;
  for (const auto &element : data_) {
    auto mnt_entry = std::dynamic_pointer_cast<MountEntry>(element.second);
    if (mnt_entry) {
      res.emplace_back(*mnt_entry);
    }
  }
  return res;
}

} // namespace usbmount::dal