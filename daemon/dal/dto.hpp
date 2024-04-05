#pragma once
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <cstddef>
#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <vector>

namespace usbmount::dal {

namespace json = boost::json;

class Dto {
public:
  Dto() = default;
  std::string Serialize() const noexcept;
  virtual boost::json::object ToJson() const noexcept = 0;
  virtual ~Dto() = default;
};

// CRUD table
class Table : public Dto {
public:
  Table(const std::string &data_file_path);
  virtual void Create(const Dto &) = 0;
  virtual const Dto &Read(size_t) = 0;
  virtual void Update(size_t, const Dto &) = 0;
  virtual void Delete(size_t) = 0;
  virtual ~Table() = default;
  virtual size_t size() const noexcept = 0;

protected:
  std::string raw_json_;

private:
  void ReadRaw();
  virtual void DataFromRawJson() = 0;

  const std::string file_path_; // path to data file
};

// --------------------------------------
// dtos

class Device : public Dto {
public:
  Device(const boost::json::object &);
  Device(Device &&) = default;
  Device() = default;

private:
  json::object ToJson() const noexcept override;

  std::string vid_;
  std::string pid_;
  std::string serial_;
};

class User : public Dto {
public:
  User(const boost::json::object &);
  User(User &&) = default;
  User() = default;

private:
  json::object ToJson() const noexcept override;
  uid_t uid_;
  std::string name_;
};

class Group : public Dto {
public:
  Group(const boost::json::object &);
  Group(Group &&) = default;
  Group() = default;

private:
  json::object ToJson() const noexcept override;

  gid_t gid_;
  std::string name_;
};

class MountEntry : public Dto {
public:
  MountEntry(const json::object &);

private:
  std::string dev_name_;
  std::string mount_point_;
  std::string fs_type_;
  json::object ToJson() const noexcept override;
};

// --------------------------------------
// tables

using dev_perm_tuple =
    std::tuple<Device, std::vector<User>, std::vector<Group>>;

class DevicePermissions : public Table {
public:
  DevicePermissions(const std::string &);

private:
  void DataFromRawJson() override;
  // json::object ToJson() const noexcept override;
  std::map<u_int64_t, dev_perm_tuple> data_;
};

struct Mountpoints : public Table {
private:
  json::object ToJson() const noexcept override;
  // std::vector<>
};

} // namespace usbmount::dal