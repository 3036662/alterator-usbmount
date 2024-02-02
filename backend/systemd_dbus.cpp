#include "systemd_dbus.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>

namespace dbus_bindings {

Systemd::Systemd() : connection{nullptr} { ConnectToSystemDbus(); }

std::optional<bool>
Systemd::IsUnitEnabled(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  std::string result;
  try {
    auto proxy = CreateProxyToSystemd(objectPath);
    auto method =
        proxy->createMethodCall(systemd_interface_manager, "GetUnitFileState");
    method << unit_name;
    auto reply = proxy->callMethod(method);
    reply >> result;
  } catch (const sdbus::Error &ex) {
    std::cerr << "[Error] Can't check if " << unit_name << " unit is enabled"
              << std::endl;
  }
  return result.empty()
             ? std::nullopt
             : std::optional<bool>(boost::contains(result, "enabled"));
}

std::optional<bool>
Systemd::IsUnitActive(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  std::string result;
  try {
    // get unit dbus path
    auto proxy = CreateProxyToSystemd(objectPath);
    auto method =
        proxy->createMethodCall(systemd_interface_manager, "LoadUnit");
    method << unit_name;
    auto reply = proxy->callMethod(method);
    sdbus::ObjectPath unit_path;
    reply >> unit_path;

    // get unit Active State
    // proxy to unit interface dbus
    auto proxy_unit = CreateProxyToSystemd(unit_path);
    auto active_state = proxy_unit->getProperty("ActiveState")
                            .onInterface(systemd_interface_unit);
    result = active_state.get<std::string>();
  } catch (const sdbus::Error &ex) {
    std::cerr << "[Error] Can't check if " << unit_name << " unit is active"
              << std::endl;
  }
  return result.empty()
             ? std::nullopt
             : std::optional<bool>(boost::starts_with(result, "active"));
}

/*******************    Private *****************************/

void Systemd::ConnectToSystemDbus() noexcept {
  try {
    connection = sdbus::createSystemBusConnection();
  } catch (const sdbus::Error &ex) {
    std::cerr << "[Error] Can't create connection to SDbus" << std::endl;
  }
}

bool Systemd::Health() {
  if (!connection)
    ConnectToSystemDbus();
  return static_cast<bool>(connection);
}

std::unique_ptr<sdbus::IProxy>
Systemd::CreateProxyToSystemd(const std::string &path) {
  return sdbus::createProxy(*connection, destinationName, path);
}

} // namespace dbus_buindings