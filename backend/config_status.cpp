#include "config_status.hpp"

namespace guard {



vecPairs ConfigStatus::SerializeForLisp() const{
  vecPairs res;
  //res.emplace_back("label_udev_rules_filename",pair.first);
  return res;
}


}