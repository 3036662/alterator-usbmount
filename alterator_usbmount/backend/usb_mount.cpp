#include "usb_mount.hpp"
#include "log.hpp"
#include <exception>
#include <sdbus-c++/sdbus-c++.h>
#include <string>

namespace alterator::usbmount {
using common_utils::Log;

UsbMount::UsbMount() noexcept : dbus_proxy_(nullptr) {
  try {
    dbus_proxy_ = sdbus::createProxy(kDest, kObjectPath);
  } catch (const std::exception &ex) {
    Log::Warning() << "[Usbmount] Can't create dbus proxy ";
  }
}

std::vector<std::string> UsbMount::ListDevices() const noexcept {
  std::vector<std::string> res;
  if (!dbus_proxy_) return res;
  dbus_proxy_->createMethodCall(kInterfaceName, "ListDevice");
  return res;
}

} // namespace alterator::usbmount