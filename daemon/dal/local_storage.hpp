/* File: local_storage.hpp

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
#include "device_permissions.hpp"
#include "dto.hpp"
#include "mount_points.hpp"
#include "table.hpp"
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace usbmount::dal {

/**
 * @brief Local storage holds data about known devices and mountpoints.
 *
 */

class LocalStorage {
public:
  LocalStorage(const LocalStorage &) = delete;
  LocalStorage(LocalStorage &&) = delete;
  LocalStorage &operator=(const LocalStorage &) = delete;
  LocalStorage &operator=(LocalStorage &&) = delete;

  /**
   * @brief Get the Storage object
   * @return LocalStorage
   */
  static std::shared_ptr<LocalStorage> GetStorage();

  DevicePermissions permissions;
  Mountpoints mount_points;

private:
  LocalStorage();

  static std::shared_ptr<LocalStorage> p_instance_;
  static std::mutex mutex_;
};

} // namespace usbmount::dal

// dao.mountpoints().add();