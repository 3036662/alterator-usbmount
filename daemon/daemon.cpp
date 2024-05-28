/* File: daemon.cpp

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
#include "dbus_methods.hpp"
#include "udev_monitor.hpp"
// #include "udisks_dbus.hpp"
#include "utils.hpp"
#include <csignal>
#include <memory>
#include <sdbus-c++/sdbus-c++.h>
#include <thread>

// NOLINTBEGIN(misc-include-cleaner)

namespace usbmount {

Daemon::Daemon()
    : is_running_(true), reload_(false),
      logger_(utils::InitLogFile("/var/log/alt-usb-automount/log.txt")),
      udev_(std::make_shared<UdevMonitor>(logger_)),
      dbus_methods_(udev_, logger_) {}

bool Daemon::IsRunning() noexcept {
  if (reload_) {
    reload_ = false;
    Daemon::Reload();
  }
  return is_running_;
}

void Daemon::SignalHandler(int signal) noexcept {
  switch (signal) {
  case SIGINT:
  case SIGTERM: {
    Daemon::instance().is_running_ = false;
    break;
  }
  case SIGHUP: { // NOLINT(misc-include-cleaner)
    Daemon::instance().reload_ = true;
    break;
  }
  default:
    break;
  }
}

void Daemon::Run() {
  dbus_methods_.Run(); // non blocking - Async
  std::thread thread_monitor(&UdevMonitor::Run, udev_.get());
  sigset_t signal_set; // NOLINT(misc-include-cleaner)
  int signal_number = 0;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  sigaddset(&signal_set, SIGTERM);
  sigaddset(&signal_set, SIGHUP);
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  sigprocmask(SIG_BLOCK, &signal_set, nullptr);
  while (IsRunning()) {
    sigwait(&signal_set, &signal_number);
    SignalHandler(signal_number);
  }
  udev_->Stop();
  thread_monitor.join();
  logger_->info("stopped the Daemon loop");
}

// NOLINTEND(misc-include-cleaner)

} // namespace usbmount