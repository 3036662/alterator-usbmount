#include "dto.hpp"
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

namespace usbmount::dal {

namespace json = boost::json;

// Dto
std::string Dto::Serialize() const noexcept {
  return json::serialize(ToJson());
}

// Device
Device::Device(const json::object &obj) {
  if (!obj.contains("vid") || !obj.contains("pid") || !obj.contains("serial") ||
      !obj.at("vid").is_string() || !obj.at("pid").is_string() ||
      !obj.at("serial").is_string())
    throw std::invalid_argument("Ill-formed device JSON object");
  vid_ = obj.at("vid").as_string();
  pid_ = obj.at("pid").as_string();
  serial_ = obj.at("serial").as_string();
}

Device::Device(const DeviceParams &params)
    : vid_(params.vid), pid_(params.pid), serial_(params.serial) {
  size_t pos = 0;
  size_t pos2 = 0;
  std::stoi(vid_, &pos, 16);
  std::stoi(pid_, &pos2, 16);
  if (pos != vid_.size() || pos2 != pid_.size())
    throw std::logic_error("Not HEX number");
}

json::value Device::ToJson() const noexcept {
  json::object obj;
  obj["vid"] = vid_;
  obj["pid"] = pid_;
  obj["serial"] = serial_;
  return obj;
}

// User
User::User(const json::object &obj) {
  if (!obj.contains("uid") || !obj.contains("name") ||
      !obj.at("uid").is_number() || !obj.at("name").is_string())
    throw std::invalid_argument("Ill-formed User JSON object");
  uid_ = obj.at("uid").to_number<uint64_t>();
  name_ = obj.at("name").as_string();
}

User::User(uid_t uid, const std::string &name) : uid_(uid), name_(name) {
  if (name.empty())
    throw std::invalid_argument("empty name");
}

json::value User::ToJson() const noexcept {
  json::object obj;
  obj["uid"] = static_cast<uint64_t>(uid_);
  obj["name"] = name_;
  return obj;
}

// Group
Group::Group(const json::object &obj) {
  if (!obj.contains("gid") || !obj.contains("name") ||
      !obj.at("gid").is_uint64() || !obj.at("name").is_string())
    throw std::invalid_argument("Ill-formed User JSON object");
  gid_ = obj.at("gid").as_uint64();
  name_ = obj.at("name").as_string();
}

json::value Group::ToJson() const noexcept {
  json::object obj;
  obj["gid"] = gid_;
  obj["name"] = name_;
  return obj;
}

// MountEntry
MountEntry::MountEntry(const json::object &obj) {
  if (!obj.contains("dev_name") || !obj.contains("mount_point") ||
      !obj.contains("fs_type") || !obj.at("dev_name").is_string() ||
      !obj.at("mount_point").is_string() || !obj.at("fs_type").is_string())
    throw std::invalid_argument("Ill-formed MounEntry object");
  dev_name_ = obj.at("dev_name").as_string();
  mount_point_ = obj.at("mount_point_").as_string();
  fs_type_ = obj.at("fs_type").as_string();
}

json::value MountEntry::ToJson() const noexcept {
  json::object obj;
  obj["dev_name"] = dev_name_;
  obj["mount_point"] = mount_point_;
  obj["fs_type"] = fs_type_;
  return obj;
}

// PermissionEntry
PermissionEntry::PermissionEntry(const json::object &obj) {
  if (!obj.contains("device") || !obj.at("device").is_object() ||
      !obj.contains("users") || !obj.at("users").is_array() ||
      !obj.contains("groups") || !obj.at("groups").is_array())
    throw std::invalid_argument("Ill-formed PermissionEntry object");
  std::runtime_error exc("Error reading data from JSON DevicePermissions");
  // array of users
  for (const json::value &user : obj.at("users").as_array()) {
    if (!user.is_object())
      throw exc;
    users_.emplace_back(user.as_object());
  }
  // array of groups
  std::vector<Group> vec_groups;
  for (const json::value &group : obj.at("groups").as_array()) {
    if (!group.is_object())
      throw exc;
    groups_.emplace_back(group.as_object());
  }
  device_ = Device(obj.at("device").as_object());
}

json::value PermissionEntry::ToJson() const noexcept {
  json::object obj;
  obj["device"] = device_.ToJson();
  json::array arr_users;
  for (const auto &user : users_)
    arr_users.emplace_back(user.ToJson());
  obj["users"] = std::move(arr_users);
  json::array arr_groups;
  for (const auto &group : groups_)
    arr_groups.emplace_back(group.ToJson());
  obj["groups"] = std::move(arr_groups);
  return obj;
}

} // namespace usbmount::dal