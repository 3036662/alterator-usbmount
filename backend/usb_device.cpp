#include "usb_device.hpp"

vecPairs UsbDevice::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("label_prsnt_usb_number", std::to_string(number));
  res.emplace_back("label_prsnt_usb_status", status);
  res.emplace_back("label_prsnt_usb_name", name);
  res.emplace_back("label_prsnt_usb_id", id);
  res.emplace_back("label_prsnt_usb_port", port);
  res.emplace_back("label_prsnt_usb_connection", connection);
  return res;
}