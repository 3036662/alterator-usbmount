/* File: udisks_dbus.hpp

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
#include <memory.h>
#include <sdbus-c++/sdbus-c++.h>

namespace usbmount {

class UdisksDbus {
public:
  UdisksDbus() = delete;
  explicit UdisksDbus(std::unique_ptr<sdbus::IConnection> &conn);

  bool ProcessDevice(const UsbUdevDevice &dev);

private:
  std::unique_ptr<sdbus::IConnection> &dbus_;
  std::unique_ptr<sdbus::IProxy> proxy_udisks_;
};

} // namespace usbmount