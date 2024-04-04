#include "dbus_methods.hpp"

DbusMethods::DbusMethods()
    : connection_(sdbus::createSystemBusConnection(service_name)),
      dbus_object_ptr(sdbus::createObject(*connection_, object_path)) {
  dbus_object_ptr->registerMethod(interface_name, "health", "", "s", Health);
  dbus_object_ptr->registerMethod(interface_name, "CanAnotherUserUnmount", "s",
                                  "s", CanAnotherUserUnmount);
  dbus_object_ptr->finishRegistration();
}

void DbusMethods::Run() { connection_->enterEventLoopAsync(); }

void DbusMethods::Health(const sdbus::MethodCall &call) {
  auto reply = call.createReply();
  reply << "OK";
  reply.send();
}

void DbusMethods::CanAnotherUserUnmount(const sdbus::MethodCall &call) {
  auto reply = call.createReply();
  reply << "YES";
  reply.send();
}
