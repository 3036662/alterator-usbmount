/* File: local_storage.cpp

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

#include "local_storage.hpp"
#include "mount_points.hpp"
#include <memory>
#include <mutex>

namespace usbmount::dal {

std::shared_ptr<LocalStorage> LocalStorage::p_instance_{nullptr};
std::mutex LocalStorage::mutex_;

std::shared_ptr<LocalStorage> LocalStorage::GetStorage() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!p_instance_) {
    p_instance_.reset(new LocalStorage());
  }
  return p_instance_;
}

LocalStorage::LocalStorage()
    : permissions("/var/lib/alt-usb-mount/permissions.json"),
      mount_points("/var/lib/alt-usb-mount/mount_points.json") {}
} // namespace usbmount::dal