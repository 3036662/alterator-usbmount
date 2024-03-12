#include "daemon.hpp"
#include <csignal>
#include <functional>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/sdbus-c++.h>

Daemon::Daemon() : is_running_(true), reload_(false) { ConnectToDBus(); }

bool Daemon::IsRunning() {
  if (reload_) {
    reload_ = false;
    Daemon::Reload();
  }
  return is_running_;
}

void Daemon::SignalHandler(int signal) {
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
  const char *serviceName = "ru.alterator.usbd";
  auto connection = sdbus::createSystemBusConnection(serviceName);
  const char *objectPath = "/ru/altusbd/altusbd";
  auto dbus_object_ptr = sdbus::createObject(*connection, objectPath);
  // sdbus::IObject* object_cptr = dbus_object_ptr.get();
  // Register D-Bus methods and signals on the concatenator object, and exports
  // the object.
  const char *interfaceName = "ru.alterator.Usbd";
  std::function<void(sdbus::MethodCall)> health_func =
      [](const sdbus::MethodCall &) {};
  dbus_object_ptr->registerMethod(interfaceName, "health", "", "s",
                                  health_func);
  dbus_object_ptr->finishRegistration();
  connection->enterEventLoop();
}