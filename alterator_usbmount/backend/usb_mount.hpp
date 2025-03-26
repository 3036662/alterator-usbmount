/* File: usb_mount.hpp

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
#include "active_device.hpp"
#include <memory>
#include <sdbus-c++/sdbus-c++.h>

namespace alterator::usbmount {

struct DbusOneParam {
  std::string method;
  std::string param;
};

class UsbMount {
public:
  UsbMount() noexcept;

  std::vector<ActiveDevice> ListDevices() const noexcept;
  std::string getRulesJson() const noexcept;
  std::string GetUsersGroups() const noexcept;
  std::string SaveRules(const std::string &) const noexcept;
  bool Health() const noexcept;
  bool Run() noexcept;
  bool Stop() noexcept;

private:
  const std::string kDest = "ru.alterator.usbd";
  const std::string kObjectPath = "/ru/alterator/altusbd";
  const std::string kInterfaceName = "ru.alterator.Usbd";
  const std::string kServiceUnitName = "altusbd.service";

  std::string GetStringNoParams(const std::string &method_name) const noexcept;
  std::string GetStringResponse(const DbusOneParam &) const noexcept;
  std::unique_ptr<sdbus::IProxy> dbus_proxy_;

  sdbus::InterfaceName interface_usbd_;
};

} // namespace alterator::usbmount
