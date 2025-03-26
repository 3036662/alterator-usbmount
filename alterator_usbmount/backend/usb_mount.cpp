/* File: usb_mount.cpp

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

#include "usb_mount.hpp"
#include "active_device.hpp"
#include "log.hpp"
#include "systemd_dbus.hpp"
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <cstddef>
#include <exception>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/sdbus-c++.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace alterator::usbmount {
using common_utils::Log;

UsbMount::UsbMount() noexcept
    : dbus_proxy_(nullptr), interface_usbd_{kInterfaceName} {
  try {
    dbus_proxy_ = sdbus::createProxy(sdbus::ServiceName{kDest},
                                     sdbus::ObjectPath{kObjectPath});
  } catch (const std::exception &ex) {
    Log::Warning() << "[Usbmount] Can't create dbus proxy ";
  }
}

std::vector<ActiveDevice> UsbMount::ListDevices() const noexcept {
  namespace json = boost::json;
  std::vector<ActiveDevice> res;
  if (!dbus_proxy_) {
    return res;
  }
  const sdbus::MemberName list_method("ListDevices");
  auto method = dbus_proxy_->createMethodCall(interface_usbd_, list_method);
  auto reply = dbus_proxy_->callMethod(method);
  std::string json_string;
  reply >> json_string;
  try {
    auto value = json::parse(json_string);
    const json::array &arr = value.as_array();
    size_t counter = 1;
    for (const auto &device : arr) {
      if (!device.is_object()) {
        throw std::runtime_error("Can't parse a Json strinh");
      }
      res.emplace_back(device.as_object());
      res.back().index = counter;
      ++counter;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Error parsing the daemon response";
    Log::Error() << ex.what();
  }
  // Log::Debug() << "[ListDevices]" << json_string;
  return res;
}

std::string
UsbMount::GetStringNoParams(const std::string &method_name) const noexcept {
  std::string res;
  if (!dbus_proxy_) {
    return res;
  }
  try {
    const sdbus::MethodName method_name_obj{method_name};
    auto method =
        dbus_proxy_->createMethodCall(interface_usbd_, method_name_obj);
    auto reply = dbus_proxy_->callMethod(method);
    reply >> res;
  } catch (const std::exception &ex) {
    Log::Error() << "[GetStringResponse] " << method_name;
    Log::Error() << ex.what();
  }

  return res;
}

std::string
UsbMount::GetStringResponse(const DbusOneParam &param) const noexcept {
  std::string res;
  if (!dbus_proxy_) {
    return res;
  }
  try {
    const sdbus::MethodName method_name_obj{param.method};
    auto method =
        dbus_proxy_->createMethodCall(interface_usbd_, method_name_obj);
    method << param.param;
    auto reply = dbus_proxy_->callMethod(method);
    reply >> res;
  } catch (const std::exception &ex) {
    Log::Error() << "[GetStringResponse] " << param.method;
    Log::Error() << ex.what();
  }
  return res;
}

std::string UsbMount::getRulesJson() const noexcept {
  return GetStringNoParams("ListRules");
}

std::string UsbMount::GetUsersGroups() const noexcept {
  return GetStringNoParams("GetUsersAndGroups");
}

std::string UsbMount::SaveRules(const std::string &data) const noexcept {
  return GetStringResponse({"SaveRules", data});
}

bool UsbMount::Health() const noexcept {
  if (!dbus_proxy_) {
    return false;
  }
  auto response = GetStringNoParams("health");
  return response == "OK";
}

bool UsbMount::Run() noexcept {
  dbus_bindings::Systemd systemd;
  auto enabled = systemd.IsUnitEnabled(kServiceUnitName);
  if (!enabled.has_value()) {
    Log::Error() << "[Run] Can't check if service is enabled";
    return false;
  }
  if (!enabled.value_or(false)) {
    auto success = systemd.EnableUnit(kServiceUnitName);
    if (!success.value_or(false)) {
      Log::Error() << "[Run] Can't enable service";
      return false;
    }
  }
  auto active = systemd.IsUnitActive(kServiceUnitName);
  if (!active.has_value()) {
    Log::Error() << "[Run] Can't check if unit is active";
    return false;
  }
  if (!active.value_or(false)) {
    auto success = systemd.StartUnit(kServiceUnitName);
    if (!success.value_or(false)) {
      Log::Error() << "[Run] Can't start the service";
      return false;
    }
  }
  try {
    dbus_proxy_ = sdbus::createProxy(sdbus::ServiceName{kDest},
                                     sdbus::ObjectPath{kObjectPath});
  } catch (const std::exception &ex) {
    Log::Warning() << "[Usbmount][Run] Can't create dbus proxy ";
    return false;
  }
  return true;
}

bool UsbMount::Stop() noexcept {
  dbus_bindings::Systemd systemd;

  auto active = systemd.IsUnitActive(kServiceUnitName);
  if (!active.has_value()) {
    Log::Error() << "[Stop] Can't check if the unit is active";
    return false;
  }
  if (active.value_or(false)) {
    auto success = systemd.StopUnit(kServiceUnitName);
    if (!success.value_or(false)) {
      Log::Error() << "[Stop] Can't stop the service";
      return false;
    }
  }
  auto enabled = systemd.IsUnitEnabled(kServiceUnitName);
  if (!enabled.has_value()) {
    Log::Error() << "[Stop] Can't check if the service is enabled";
    return false;
  }
  if (enabled.value_or(false)) {
    auto success = systemd.DisableUnit(kServiceUnitName);
    if (!success.value_or(false)) {
      Log::Error() << "[Stop] Can't disable the service";
      return false;
    }
  }
  dbus_proxy_ = nullptr;
  return true;
}

} // namespace alterator::usbmount