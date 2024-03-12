
#include "daemon.hpp"
#include <iostream>
#include <systemd/sd-daemon.h>
#include <thread>

// grab a bus name as last step of initialization.

// make your daemon bus-activatable by supplying
// a D-Bus service activation configuration file

int main() {
  Daemon &daemon = Daemon::instance();
  // Daemon main loop
  while (daemon.IsRunning()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  std::cerr << SD_DEBUG << "The daemon process ended gracefully.";
  return 0;
}