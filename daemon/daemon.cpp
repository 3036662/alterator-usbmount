#include "daemon.hpp"
#include "udev_monitor.hpp"
#include "udisks_dbus.hpp"
#include "usb_udev_device.hpp"
#include <csignal>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/StandardInterfaces.h>
#include <sdbus-c++/sdbus-c++.h>

Daemon::Daemon() : is_running_(true), reload_(false) {
  signal(SIGINT, Daemon::SignalHandler);
  signal(SIGTERM, Daemon::SignalHandler);
  signal(SIGHUP, Daemon::SignalHandler);
  ConnectToDBus();
  udev_ = std::make_unique<UdevMonitor>(UdevMonitor());
  udisks_ = std::make_unique<UdisksDbus>(connection_);
}

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
  connection_ = sdbus::createSystemBusConnection(serviceName);
  const char *objectPath = "/ru/alterator/altusbd";
  auto dbus_object_ptr = sdbus::createObject(*connection_, objectPath);
  // Register D-Bus methods and signals on the concatenator object, and exports
  // the object.
  const char *interfaceName = "ru.alterator.Usbd";
  std::function<void(sdbus::MethodCall)> health_func =
      [](const sdbus::MethodCall &call) {
        auto reply = call.createReply();
        reply << "OK";
        reply.send();
      };
  dbus_object_ptr->registerMethod(interfaceName, "health", "", "s",
                                  health_func);
  dbus_object_ptr->finishRegistration();
  connection_->enterEventLoopAsync();
}

void Daemon::CheckEvents() {
  std::optional<UsbUdevDevice> device = udev_->RecieveDevice();
  if (device.has_value())
    udisks_->ProcessDevice(*device);
}