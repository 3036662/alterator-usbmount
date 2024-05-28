/* File: dbus_methods.hpp

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
#include "dal/local_storage.hpp"
#include "udev_monitor.hpp"
#include <boost/json/array.hpp>
#include <memory>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/sdbus-c++.h>
#include <spdlog/logger.h>
#include <string>

namespace usbmount {

class DbusMethods {
public:
  DbusMethods(const DbusMethods &) = delete;
  DbusMethods(DbusMethods &&) = delete;
  DbusMethods &operator=(const DbusMethods &) = delete;
  DbusMethods &&operator=(DbusMethods &&) = delete;
  DbusMethods() = delete;
  ~DbusMethods() = default;
  explicit DbusMethods(std::shared_ptr<UdevMonitor> udev_monitor,
                       std::shared_ptr<spdlog::logger> logger);

  void Run();

private:
  /** @brief Health method for DBus returns "OK" to caller */
  static void Health(const sdbus::MethodCall &);

  void CanAnotherUserUnmount(sdbus::MethodCall);
  void CanUserMount(sdbus::MethodCall);
  void ListActiveDevices(const sdbus::MethodCall &);
  void ListActiveRules(const sdbus::MethodCall &);
  void GetSystemUsersAndGroups(const sdbus::MethodCall &);
  void SaveRules(sdbus::MethodCall);

  void UpdateRules(const boost::json::array &arr_updated);
  void CreateRules(const boost::json::array &arr_created);

  const std::string service_name = "ru.alterator.usbd";
  const std::string object_path = "/ru/alterator/altusbd";
  const std::string interface_name = "ru.alterator.Usbd";
  std::unique_ptr<sdbus::IConnection> connection_;
  std::unique_ptr<sdbus::IObject> dbus_object_ptr;
  std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<dal::LocalStorage> dbase_;
  std::shared_ptr<UdevMonitor> udev_monitor_;
};

} // namespace usbmount