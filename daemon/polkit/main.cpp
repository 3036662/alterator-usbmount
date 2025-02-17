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

#include <exception>
#include <iostream>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/sdbus-c++.h>
#include <string>

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    std::cout << "EMPTY params";
    return 0;
  }
  // NOLINTBEGIN
  const std::string dev = argv[1];
  const std::string action = argv[2];
  // NOLINTEND
  if (dev.empty() || action.empty()) {
    std::cout << "EMPTY params";
    return 0;
  }
  const std::string dest = "ru.alterator.usbd";
  const std::string object_path = "/ru/alterator/altusbd";
  const std::string interface_name = "ru.alterator.Usbd";
  const sdbus::InterfaceName interface_name_obj{interface_name};

  try {
    auto proxy = sdbus::createProxy(sdbus::ServiceName{dest},
                                    sdbus::ObjectPath{object_path});
    if (action == "unmount") {
      const sdbus::MethodName canunmount_method{"CanAnotherUserUnmount"};
      auto method =
          proxy->createMethodCall(interface_name_obj, canunmount_method);
      method << dev;
      auto reply = proxy->callMethod(method);
      std::string res;
      reply >> res;
      std::cout << res;
    } else if (action == "mount") {
      const sdbus::MethodName canmount_method{"CanUserMount"};
      auto method =
          proxy->createMethodCall(interface_name_obj, canmount_method);
      method << dev;
      auto reply = proxy->callMethod(method);
      std::string res;
      reply >> res;
      std::cout << res;
    }
  } catch (const std::exception &ex) {
    std::cout << "Exception";
    return 0;
  }

  return 0;
}