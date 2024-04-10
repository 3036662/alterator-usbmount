#include "dbus_methods.hpp"
#include "dal/local_storage.hpp"
#include <string>

namespace usbmount {

DbusMethods::DbusMethods()
    : connection_(sdbus::createSystemBusConnection(service_name)),
      dbus_object_ptr(sdbus::createObject(*connection_, object_path)) {
  dbus_object_ptr->registerMethod(interface_name, "health", "", "s", Health);
  dbus_object_ptr->registerMethod(interface_name, "CanAnotherUserUnmount", "s",
                                  "s", CanAnotherUserUnmount);
  dbus_object_ptr->registerMethod(interface_name, "CanUserMount", "s", "s",
                                  CanUserMount);
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
  TODO query local db if drive is in db  - reply YES
  */
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
  auto reply = call.createReply();
  reply << "NO";
  reply.send();
}

} // namespace usbmount