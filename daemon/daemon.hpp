/* File: daemon.hpp

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
#include "udev_monitor.hpp"
// #include "udisks_dbus.hpp"
#include "dbus_methods.hpp"
#include <memory>
#include <sdbus-c++/sdbus-c++.h>
#include <spdlog/logger.h>

namespace usbmount {

class Daemon {
public:
  Daemon(Daemon const &) = delete;
  Daemon(Daemon const &&) = delete;
  Daemon &operator=(const Daemon &) = delete;
  Daemon &operator=(Daemon &&) = delete;
  ~Daemon() = default;

  /**
   * @brief Create a daemon instance
   * @return Daemon&
   * @throws sdbus::Error  std::runtime_error
   */
  static Daemon &instance() {
    static Daemon instance;
    return instance;
  }

  void Run();

  // void CheckEvents();

private:
  Daemon();

  bool IsRunning() noexcept;
  static void SignalHandler(int signal) noexcept;
  static void Reload() noexcept {};

  bool is_running_;
  bool reload_;
  std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<UdevMonitor> udev_;
  DbusMethods dbus_methods_; // default construction

  // std::unique_ptr<UdisksDbus> udisks_;
};

} // namespace usbmount