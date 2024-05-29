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

#include "dispatcher_impl.hpp"
#include "lisp_message.hpp"
#include "message_reader.hpp"
#include "usb_mount.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  namespace um = ::alterator::usbmount;
  um::UsbMount mount;
  um::DispatcherImpl dispatcher(mount);
  auto dispatcher_func = [&dispatcher](const LispMessage &msg) {
    return dispatcher.Dispatch(msg);
  };
  const MessageReader reader(dispatcher_func);
  reader.Loop();
  return 0;
}