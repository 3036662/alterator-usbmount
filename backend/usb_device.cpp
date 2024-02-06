#include "usb_device.hpp"
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>

namespace guard {

UsbDevice::UsbDevice(int num, const std::string &status_,
                     const std::string &name_, const std::string &vid_,
                     const std::string &pid_, const std::string &port_,
                     const std::string &connection_, const std::string &i_type_)
    : number(num), status(status_), name(name_), vid(vid_), pid(pid_),
      port(port_), connection(connection_), i_type(i_type_) {}

vecPairs UsbDevice::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("label_prsnt_usb_number", std::to_string(number));
  res.emplace_back("label_prsnt_usb_port", port);
  res.emplace_back("label_prsnt_usb_class", i_type);
  res.emplace_back("label_prsnt_usb_vid", vid);
  res.emplace_back("label_prsnt_usb_pid", pid);
  res.emplace_back("label_prsnt_usb_status", status);
  res.emplace_back("label_prsnt_usb_name", name);
  res.emplace_back("label_prsnt_usb_connection", connection);
  return res;
}

UsbType::UsbType(const std::string &str) {
  std::logic_error ex = std::logic_error("Can't parse CC::SS::PP " + str);
  std::vector<std::string> splitted;
  boost::split(splitted, str, [](const char c) { return c == ':'; });
  if (splitted.size() != 3) {
    throw ex;
  }
  for (std::string &el : splitted) {
    boost::trim(el);
    if (el.size() > 2) {
      throw ex;
    }
  }
  base = stoi(splitted[0], nullptr, 16);
  base_str = std::move(splitted[0]);
  sub = stoi(splitted[1], nullptr, 16);
  sub_str = std::move(splitted[1]);
  protocol = stoi(splitted[2], nullptr, 16);
  protocol_str = std::move(splitted[2]);
}

} // namespace guard