#pragma once
#include "udev_monitor.hpp"
#include "udisks_dbus.hpp"
#include <memory>
#include <sdbus-c++/sdbus-c++.h>

class Daemon {
public:
  static Daemon &instance() {
    static Daemon instance;
    return instance;
  }

  bool IsRunning();
  void CheckEvents();

private:
  Daemon();
  Daemon(Daemon const &) = delete;
  Daemon(Daemon const &&) = delete;
  void operator=(Daemon const &) = delete;

  void ConnectToDBus();
  static void SignalHandler(int signal);
  static void Reload(){};

  bool is_running_;
  bool reload_;
  std::unique_ptr<sdbus::IConnection> connection_;
  std::unique_ptr<UdevMonitor> udev_;
  std::unique_ptr<UdisksDbus> udisks_;
};