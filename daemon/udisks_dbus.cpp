#include "udisks_dbus.hpp"
#include <iostream>
#include <sdbus-c++/Types.h>
#include <thread>
#include <vector>

UdisksDbus::UdisksDbus(std::unique_ptr<sdbus::IConnection> &conn)
    : dbus_(conn),
      proxy_udisks_(sdbus::createProxy(*dbus_, "org.freedesktop.UDisks2",
                                       "/org/freedesktop/UDisks2/Manager")) {}

bool UdisksDbus::ProcessDevice(const UsbUdevDevice &dev) {
  std::cerr << dev.block_name << "\n";
  auto method = proxy_udisks_->createMethodCall(
      "org.freedesktop.UDisks2.Manager", "ResolveDevice");
  std::map<std::string, sdbus::Variant> param;
  param.emplace("path", dev.block_name);
  method << param << std::map<std::string, sdbus::Variant>();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  auto reply = proxy_udisks_->callMethod(method);
  std::vector<sdbus::ObjectPath> obj_path;
  reply >> obj_path;
  std::cerr << obj_path.size() << "\n";
  if (!obj_path.empty())
    std::cerr << obj_path[0] << "\n";
  return true;
}