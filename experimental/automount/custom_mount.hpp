/* File: custom_mount.hpp  

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

/**
 * @brief Options for mount() glibc call
 */
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

  /**
   * @brief Mount a device for user and group
   * @param uid_gid user and group struct
   */
  bool Mount(const UidGid &uid_gid) noexcept;

  /**
   * @brief Unmount a device
   */
  bool UnMount() noexcept;

private:
  /**
   * @brief Create a Acl-controlled directory for mount points
   */
  bool CreateAclMountPoint() noexcept;

  /**
   * @brief Set the Acl for directory
   * @param mount_point
   */
  void SetAcl(const std::string &mount_point);

  /**
   * @brief Create a subfolder in ACL-controlled dir for mounting
   */
  bool CreatMountEndpoint() noexcept;

  bool PerfomMount() noexcept;
  void RemoveMountPoint(const std::string &path) noexcept;

  /**
   * @brief Set the Mount Options object - paramterers for mount call
   * @param[in][out] opts
   */
  void SetMountOptions(MountOptions &opts) const noexcept;

  const char *mount_root = "/media/";
  const std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<UsbUdevDevice> ptr_device_;

  std::optional<uid_t> uid_;
  std::optional<gid_t> gid_;
  std::optional<std::string> base_mount_point_; // base mount point with acl
  std::optional<std::string> end_mount_point_;  // child dir for mounting
};