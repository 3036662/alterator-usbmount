/* File: dto.hpp

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

#pragma once
// NOLINTNEXTLINE
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <memory>
#include <string>
// NOLINTNEXTLINE
#include <sys/types.h>
#include <vector>

namespace usbmount::dal {

namespace json = boost::json;

class Dto {
public:
  Dto() = default;
  Dto(const Dto &) = default;
  Dto(Dto &&) = default;
  Dto &operator=(const Dto &) = default;
  Dto &operator=(Dto &&) = default;
  virtual ~Dto() = default;
  std::string Serialize() const noexcept;
  virtual boost::json::value ToJson() const noexcept = 0;
  virtual std::shared_ptr<Dto> Clone() const noexcept = 0;
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
  explicit Device(const DeviceParams &params);
  Device(Device &&) = default;
  Device(const Device &) = default;
  Device() = default;
  Device &operator=(Device &&) noexcept = default;
  Device &operator=(const Device &) noexcept = default;
  bool operator==(const Device &) const noexcept;
  ~Device() override = default;

  json::value ToJson() const noexcept override;

  inline std::shared_ptr<Dto> Clone() const noexcept override {
    return std::make_shared<Device>(*this);
  };

  inline const std::string &vid() const noexcept { return vid_; }
  inline const std::string &pid() const noexcept { return pid_; }
  inline const std::string &serial() const noexcept { return serial_; }

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
  // NOLINTNEXTLINE
  User(uid_t uid, const std::string &name);
  ~User() override = default;

  json::value ToJson() const noexcept override;
  inline uid_t uid() const noexcept { return uid_; }
  const std::string &name() const noexcept { return name_; }

  inline std::shared_ptr<Dto> Clone() const noexcept override {
    return std::make_shared<User>(*this);
  };

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
  // NOLINTNEXTLINE
  Group(gid_t gid, const std::string &name);
  ~Group() override = default;

  json::value ToJson() const noexcept override;
  inline gid_t gid() const noexcept { return gid_; }
  const std::string &name() const noexcept { return name_; }

  inline std::shared_ptr<Dto> Clone() const noexcept override {
    return std::make_shared<Group>(*this);
  };

private:
  gid_t gid_ = 0;
  std::string name_;
};

struct MountEntryParams {
  std::string dev_name;
  std::string mount_point;
  std::string fs;
};

class MountEntry : public Dto {
public:
  explicit MountEntry(const json::object &);
  explicit MountEntry(const MountEntryParams &params);
  MountEntry() = default;
  MountEntry(const MountEntry &) = default;
  MountEntry(MountEntry &&) = default;
  MountEntry &operator=(MountEntry &&) noexcept = default;
  MountEntry &operator=(const MountEntry &) noexcept = default;
  bool operator==(const MountEntry &other) const noexcept;
  ~MountEntry() override = default;

  json::value ToJson() const noexcept override;
  inline const std::string &dev_name() const noexcept { return dev_name_; }
  inline const std::string &mount_point() const noexcept {
    return mount_point_;
  }

  inline std::shared_ptr<Dto> Clone() const noexcept override {
    return std::make_shared<MountEntry>(*this);
  };

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
  PermissionEntry &operator=(const PermissionEntry &) = default;
  PermissionEntry &operator=(PermissionEntry &&) = default;
  PermissionEntry(Device &&dev, std::vector<User> &&users,
                  std::vector<Group> &&groups);
  ~PermissionEntry() override = default;

  json::value ToJson() const noexcept override;

  inline const Device &getDevice() const noexcept { return device_; }
  inline const std::vector<User> &getUsers() const noexcept { return users_; }
  inline const std::vector<Group> &getGroups() const noexcept {
    return groups_;
  }
  inline std::shared_ptr<Dto> Clone() const noexcept override {
    return std::make_shared<PermissionEntry>(*this);
  };

private:
  Device device_;
  std::vector<User> users_;
  std::vector<Group> groups_;
};

} // namespace usbmount::dal