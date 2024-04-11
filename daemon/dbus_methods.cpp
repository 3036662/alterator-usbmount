#include "dbus_methods.hpp"
#include "dal/local_storage.hpp"
#include "usb_udev_device.hpp"
#include <exception>
#include <sdbus-c++/Message.h>
#include <string>

namespace usbmount {

DbusMethods::DbusMethods(std::shared_ptr<spdlog::logger> logger)
    : connection_(sdbus::createSystemBusConnection(service_name)),
      dbus_object_ptr(sdbus::createObject(*connection_, object_path)),
      logger_(std::move(logger)), dbase_(dal::LocalStorage::GetStorage()) {
  dbus_object_ptr->registerMethod(interface_name, "health", "", "s", Health);
  dbus_object_ptr->registerMethod(interface_name, "CanAnotherUserUnmount", "s",
                                  "s", [this](sdbus::MethodCall call) {
                                    CanAnotherUserUnmount(std::move(call));
                                  });
  dbus_object_ptr->registerMethod(
      interface_name, "CanUserMount", "s", "s",
      [this](sdbus::MethodCall call) { CanUserMount(std::move(call)); });
  dbus_object_ptr->finishRegistration();
}

void DbusMethods::Run() { connection_->enterEventLoopAsync(); }

void DbusMethods::Health(const sdbus::MethodCall &call) {
  auto reply = call.createReply();
  reply << "OK";
  reply.send();
}

void DbusMethods::CanAnotherUserUnmount(sdbus::MethodCall call) {
  std::string dev;
  call >> dev;
  auto reply = call.createReply();
  /*
  TODO query local db if drive is in db (is mounted by daemon)  - reply YES
  */
  logger_->debug("Polkit request for device (CanUserUnMount)" + dev);
  logger_->flush();
  reply << "YES";
  reply.send();
}

void DbusMethods::CanUserMount(sdbus::MethodCall call) {
  std::string dev;
  call >> dev;
  /*
  TODO read local db define is this user is allowed to mount this drive
  (if drive is in db - reply NO - automount will mount it )
  */
  logger_->debug("Polkit request for device (CanUserMount)" + dev);
  sdbus::MethodReply reply = call.createReply();
  try {
    UsbUdevDevice device({dev, "add"});
    dal::Device dto_device({device.vid(), device.pid(), device.serial()});
    if (dbase_->permissions.Find(dto_device)) {
      logger_->debug("daemon reply to polkit = NO");
      reply << "NO";
    } else {
      logger_->debug("daemon reply to polkit = YES");
      reply << "YES";
    }
    logger_->flush();
    reply.send();
    return;
  } catch (const std::exception &ex) {
    logger_->error("[Dbus::CanUserMount]Can't construct udev device from " +
                   dev);
  }
  // just in case  -must be unreacheable
  reply << "YES";
  reply.send();
}

} // namespace usbmount