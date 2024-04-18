#pragma once
#include "active_device.hpp"
#include <memory>
#include <sdbus-c++/sdbus-c++.h>

namespace alterator::usbmount {

class UsbMount {
public:
  UsbMount() noexcept;

  std::vector<ActiveDevice> ListDevices() const noexcept;

private:
  const std::string kDest = "ru.alterator.usbd";
  const std::string kObjectPath = "/ru/alterator/altusbd";
  const std::string kInterfaceName = "ru.alterator.Usbd";

  std::unique_ptr<sdbus::IProxy> dbus_proxy_;
};

} // namespace alterator::usbmount
