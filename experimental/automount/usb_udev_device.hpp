/* File: usb_udev_device.hpp  

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
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>

struct DevParams {
  std::string dev_path;
  std::string action;
};

enum class Action { kRemove, kAdd, kUndefined };

class UsbUdevDevice {
public:
  UsbUdevDevice() = delete;

  /**
   * @brief Construct a new Usb Udev Device object
   *
   * @param params DevParams struct {dev_path,action}
   * @param logger
   * @throws std::runtime_error
   */
  UsbUdevDevice(const DevParams &params,
                const std::shared_ptr<spdlog::logger> &logger);

  std::string toString() const noexcept;
  inline const std::string &fs_label() const noexcept { return fs_label_; }
  inline const std::string &fs_uid() const noexcept { return uid_; }
  inline const std::string &filesystem() const noexcept { return filesystem_; }
  inline const std::string &dev_type() const noexcept { return dev_type_; }
  inline const std::string &block_name() const noexcept { return block_name_; }
  inline int partition_number() const noexcept { return partitions_number_; }

private:
  /**
   * @brief Get the Udev Device Info for block device
   */
  void getUdevDeviceInfo();

  const std::shared_ptr<spdlog::logger> logger_;
  Action action_;
  std::string block_name_;
  std::string fs_label_;   /// ID_FS_LABEL
  std::string uid_;        /// ID_FS_UUID
  std::string filesystem_; /// ID_FS_TYPE
  std::string dev_type_;   /// DEVTYPE e.g "partition"
  int partitions_number_;
  // std::string vid_;
  // std::string pid_;
  // std::string subsystem_;
};