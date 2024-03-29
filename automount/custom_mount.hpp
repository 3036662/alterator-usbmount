#pragma once
#include "usb_udev_device.hpp"
#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <sys/types.h>

struct UidGid {
  uid_t user_id;
  gid_t group_id;
};

class CustomMount {
public:
  CustomMount() = delete;
  CustomMount(const CustomMount &) = delete;
  CustomMount(CustomMount &&) = delete;
  CustomMount &operator=(const CustomMount &) = delete;
  CustomMount &&operator=(CustomMount &&) = delete;

  CustomMount(const std::shared_ptr<spdlog::logger> &logger) noexcept;

  bool Mount(std::shared_ptr<UsbUdevDevice> &ptr_device,
             const UidGid &uid_gid) noexcept;

private:
  bool CreateAclMountPoint() noexcept;

  void SetAcl(const std::string &mount_point);

  const char *mount_root = "/run/alt-usb-mount/";
  const std::shared_ptr<spdlog::logger> logger_;
  std::optional<uid_t> uid_;
  std::optional<gid_t> gid_;
};