#pragma once
#include "udev_monitor.hpp"
// #include "udisks_dbus.hpp"
#include <memory>
#include <sdbus-c++/sdbus-c++.h>
#include <spdlog/logger.h>

class Daemon {
public:
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
  Daemon(Daemon const &) = delete;
  Daemon(Daemon const &&) = delete;
  Daemon &operator=(const Daemon &) = delete;
  Daemon &operator=(Daemon &&) = delete;

  bool IsRunning() noexcept;
  void ConnectToDBus();
  void StartDbusLoop();

  static void SignalHandler(int signal) noexcept;
  static void Reload() noexcept {};

  bool is_running_;
  bool reload_;
  std::shared_ptr<spdlog::logger> logger_;
  std::unique_ptr<sdbus::IConnection> connection_;
  std::unique_ptr<UdevMonitor> udev_;
  std::unique_ptr<sdbus::IObject> dbus_object_ptr;

  // std::unique_ptr<UdisksDbus> udisks_;
};