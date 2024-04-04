#include "daemon.hpp"
#include "udev_monitor.hpp"
// #include "udisks_dbus.hpp"
#include "utils.hpp"
#include <csignal>
#include <memory>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/StandardInterfaces.h>
#include <sdbus-c++/sdbus-c++.h>
#include <thread>

Daemon::Daemon()
    : is_running_(true), reload_(false),
      logger_(utils::InitLogFile("/var/log/alt-usb-automount/log.txt")) {
  ConnectToDBus();
  udev_ = std::make_unique<UdevMonitor>(UdevMonitor(logger_));
  //  not needed yet
  //  udisks_ = std::make_unique<UdisksDbus>(connection_);
}

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
  case SIGHUP: {
    Daemon::instance().reload_ = true;
    break;
  }
  default:
    break;
  }
}

void Daemon::ConnectToDBus() {
  const char *service_name = "ru.alterator.usbd";
  connection_ = sdbus::createSystemBusConnection(service_name);
  const char *object_path = "/ru/alterator/altusbd";
  dbus_object_ptr = sdbus::createObject(*connection_, object_path);
  const char *interfaceName = "ru.alterator.Usbd";
  auto health_func = [](const sdbus::MethodCall &call) {
    auto reply = call.createReply();
    reply << "OK";
    reply.send();
  };
  dbus_object_ptr->registerMethod(interfaceName, "health", "", "s",
                                  health_func);
  dbus_object_ptr->finishRegistration();
}

void Daemon::Run() {
  StartDbusLoop();
  std::thread thread_monitor(&UdevMonitor::Run, udev_.get());
  sigset_t signal_set;
  int signal_number;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  sigaddset(&signal_set, SIGTERM);
  sigaddset(&signal_set, SIGHUP);
  sigprocmask(SIG_BLOCK, &signal_set, NULL);
  while (IsRunning()) {
    sigwait(&signal_set, &signal_number);
    SignalHandler(signal_number);
  }
  udev_->Stop();
  thread_monitor.join();
  logger_->debug("stopped the Daemon loop");
}

void Daemon::StartDbusLoop() { connection_->enterEventLoopAsync(); }
