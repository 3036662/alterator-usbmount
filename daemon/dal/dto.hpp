#pragma once
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <sys/types.h>
#include <vector>

namespace usbmount::dal {

namespace json = boost::json;

class Dto {
public:
  Dto() = default;
  std::string Serialize() const noexcept;
  virtual boost::json::value ToJson() const noexcept = 0;
  virtual ~Dto() = default;
};

// --------------------------------------
// dtos

struct DeviceParams {
  std::string vid;
  std::string pid;
  std::string serial;
};

class Device : public Dto {
public:
  explicit Device(const boost::json::object &);
  Device(Device &&) = default;
  Device(const Device &) = default;
  Device() = default;
  Device &operator=(Device &&) noexcept = default;
  Device &operator=(const Device &) noexcept = default;
  Device(const DeviceParams &params);
  bool operator==(const Device &) const noexcept;

  json::value ToJson() const noexcept override;

private:
  std::string vid_;
  std::string pid_;
  std::string serial_;
};

class User : public Dto {
public:
  explicit User(const boost::json::object &);
  User(User &&) = default;
  User(const User &) = default;
  User() = default;
  User &operator=(const User &) noexcept = default;
  User &operator=(User &&) noexcept = default;
  User(uid_t uid, const std::string &name);

  json::value ToJson() const noexcept override;

private:
  uid_t uid_ = 0;
  std::string name_;
};

class Group : public Dto {
public:
  explicit Group(const boost::json::object &);
  Group(Group &&) = default;
  Group(const Group &) = default;
  Group() = default;
  Group &operator=(const Group &) noexcept = default;
  Group &operator=(Group &&) noexcept = default;
  Group(gid_t gid, const std::string &name);

  json::value ToJson() const noexcept override;

private:
  gid_t gid_ = 0;
  std::string name_;
};

struct MountEntryParams {
  const std::string &dev_name;
  const std::string &mount_point;
  const std::string &fs;
};

class MountEntry : public Dto {
public:
  explicit MountEntry(const json::object &);
  MountEntry() = default;
  MountEntry(const MountEntry &) = default;
  MountEntry(MountEntry &&) = default;
  MountEntry &operator=(MountEntry &&) noexcept = default;
  MountEntry &operator=(const MountEntry &) noexcept = default;
  MountEntry(const MountEntryParams &params);

  json::value ToJson() const noexcept override;

private:
  std::string dev_name_;
  std::string mount_point_;
  std::string fs_type_;
};

class PermissionEntry : public Dto {
public:
  explicit PermissionEntry(const json::object &);
  PermissionEntry(const PermissionEntry &) = default;
  PermissionEntry(PermissionEntry &&) = default;
  PermissionEntry &operator=(PermissionEntry &) = default;
  PermissionEntry &operator=(PermissionEntry &&) = default;
  PermissionEntry(Device &&dev, std::vector<User> &&users,
                  std::vector<Group> &&groups);

  json::value ToJson() const noexcept override;

  inline const Device &getDevice() const noexcept { return device_; }

private:
  Device device_;
  std::vector<User> users_;
  std::vector<Group> groups_;
};

} // namespace usbmount::dal