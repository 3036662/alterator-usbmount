#include "dto.hpp"
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace usbmount::dal {

namespace json = boost::json;
namespace fs = std::filesystem;

// -------------------------------------
// Dto

std::string Dto::Serialize() const noexcept {
  return json::serialize(ToJson());
}

// -------------------------------------
// CRUD Table
Table::Table(const std::string &data_file_path) : file_path_(data_file_path) {
  if (!fs::exists(file_path_)) {
    fs::create_directories(fs::path(file_path_).parent_path());
    std::ofstream file(file_path_);
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

// --------------------------------------
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

json::object Device::ToJson() const noexcept {
  json::object obj;
  obj["vid"] = vid_;
  obj["pid"] = pid_;
  obj["serial"] = serial_;
  return obj;
}

// --------------------------------------
// User

User::User(const json::object &obj) {
  if (!obj.contains("uid") || !obj.contains("name") ||
      !obj.at("uid").is_uint64() || !obj.at("name").is_string())
    throw std::invalid_argument("Ill-formed User JSON object");
  uid_ = obj.at("uid").as_uint64();
  name_ = obj.at("name").as_string();
}

json::object User::ToJson() const noexcept {
  json::object obj;
  obj["uid"] = uid_;
  obj["name"] = name_;
  return obj;
}

// --------------------------------------
// Group

Group::Group(const json::object &obj) {
  if (!obj.contains("gid") || !obj.contains("name") ||
      !obj.at("gid").is_uint64() || !obj.at("name").is_string())
    throw std::invalid_argument("Ill-formed User JSON object");
  gid_ = obj.at("gid").as_uint64();
  name_ = obj.at("name").as_string();
}

json::object Group::ToJson() const noexcept {
  json::object obj;
  obj["gid"] = gid_;
  obj["name"] = name_;
  return obj;
}

// --------------------------------------
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

json::object MountEntry::ToJson() const noexcept {
  json::object obj;
  obj["dev_name"] = dev_name_;
  obj["mount_point"] = mount_point_;
  obj["fs_type"] = fs_type_;
  return obj;
}

// --------------------------------------
// Tables

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
    Device dev(obj.at("device").as_object());
    // array of users
    std::vector<User> vec_users;
    for (const json::value &user : obj.at("users").as_array()) {
      if (!user.is_object())
        throw exc;
      vec_users.emplace_back(user.as_object());
    }
    // array of groups
    std::vector<Group> vec_groups;
    for (const json::value &group : obj.at("groups").as_array()) {
      if (!group.is_object())
        throw exc;
      vec_groups.emplace_back(group.as_object());
    }
    data_.emplace(
        std::make_pair(obj.at("id").as_uint64(),
                       std::make_tuple(std::move(dev), std::move(vec_users),
                                       std::move(vec_groups))));
  }
}

} // namespace usbmount::dal