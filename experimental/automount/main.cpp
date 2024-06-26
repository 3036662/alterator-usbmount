/* File: main.cpp  

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

// define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include "custom_mount.hpp"
#include "usb_udev_device.hpp"
#include "utils.hpp"
#include <exception>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
  // init logger
  auto logger = utils::InitLogFile("/var/log/alt-usb-automount/log.txt");
  if (!logger) {
    return 1;
  }
  // check args
  if (argc < 3) {
    logger->error("Empty args");
    return 1;
  }
  // find udev device info
  DevParams dev_params{argv[1], argv[2]};
  if (dev_params.action.empty() && dev_params.dev_path.empty()) {
    logger->error("Empty argument value");
    return 1;
  }
  dev_params.dev_path = "/dev/" + dev_params.dev_path;
  std::shared_ptr<UsbUdevDevice> ptr_device;
  try {
    ptr_device = std::make_shared<UsbUdevDevice>(dev_params, logger);
  } catch (const std::exception &ex) {
    logger->error(ex.what());
    return 1;
  }
  // device info is ready - act
  uid_t user_id = 1000;
  gid_t group_id = 1001;
  CustomMount mounter{ptr_device, logger};
  if (dev_params.action == "add") {
    mounter.Mount({user_id, group_id});
  } else if (dev_params.action == "remove") {
    mounter.UnMount();
  }

  return 0;
}