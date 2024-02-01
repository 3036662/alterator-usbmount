#include "config_status.hpp"

namespace guard {



vecPairs ConfigStatus::SerializeForLisp() const{
  vecPairs res;
  res.emplace_back("udev", udev_rules_OK ?  "OK":"BAD");
  res.emplace_back("usbguard",guard_daemon_OK ? "OK":"BAD");
  return res;
}


}