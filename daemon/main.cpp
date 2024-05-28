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

#include "daemon.hpp"
#include <exception>
#include <iostream>
#include <systemd/sd-daemon.h>

// grab a bus name as last step of initialization.

// make your daemon bus-activatable by supplying
// a D-Bus service activation configuration file

int main() {
  try {
    usbmount::Daemon &daemon = usbmount::Daemon::instance();
    daemon.Run();
  } catch (const std::exception &ex) {
    std::cerr << SD_ERR << "Can't start the daemon ";
    std::cerr << SD_ERR << ex.what();
  }
  std::cerr << SD_DEBUG << "The daemon process ended gracefully.";
  return 0;
}