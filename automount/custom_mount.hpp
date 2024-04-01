#pragma once
#include "usb_udev_device.hpp"
#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <string>
#include <sys/types.h>

struct UidGid {
  uid_t user_id;
  gid_t group_id;
};

struct MountOptions {
  unsigned long mount_flags = 0;
  std::string fs;
  std::string mount_data;
  bool read_only = false;
};

class CustomMount {
public:
  CustomMount() = delete;
  CustomMount(const CustomMount &) = delete;
  CustomMount(CustomMount &&) = delete;
  CustomMount &operator=(const CustomMount &) = delete;
  CustomMount &&operator=(CustomMount &&) = delete;

  CustomMount(std::shared_ptr<UsbUdevDevice> &ptr_device,
              const std::shared_ptr<spdlog::logger> &logger) noexcept;

  bool Mount(const UidGid &uid_gid) noexcept;
  bool UnMount() noexcept;

private:
  bool CreateAclMountPoint() noexcept;
  void SetAcl(const std::string &mount_point);
  bool CreatMountEndpoint() noexcept;
  bool PerfomMount() noexcept;
  void RemoveMountPoint(const std::string &path) noexcept;
  void SetMountOptions(MountOptions &opts) const noexcept;

  const char *mount_root = "/run/alt-usb-mount/";
  const std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<UsbUdevDevice> ptr_device_;

  std::optional<uid_t> uid_;
  std::optional<gid_t> gid_;
  std::optional<std::string> base_mount_point_; // base mount point with acl
  std::optional<std::string> end_mount_point_;  // child dir for mounting
};