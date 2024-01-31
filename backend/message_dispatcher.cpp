#include "message_dispatcher.hpp"
#include "utils.hpp"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

MessageDispatcher::MessageDispatcher(guard::Guard &guard) : guard(guard) {}

bool MessageDispatcher::Dispatch(const LispMessage &msg) {
  std::cerr << msg << std::endl;
  std::cerr << "curr action " << msg.action << std::endl;

  // list usbs
  if (msg.action == "list" && msg.objects == "list_curr_usbs") {
    std::cerr << "dispatcher->list" << std::endl;
    //  std::vector<UsbDevice> vec_usb=fakeLibGetUsbList();
    std::vector<UsbDevice> vec_usb = guard.ListCurrentUsbDevices();
    std::cout << mess_beg;
    for (const auto &usb : vec_usb) {
      std::cout << ToLisp(usb);
    }
    std::cout << mess_end;
    return true;
  }

  // allow device with id
  if (msg.action == "read" && msg.objects == "usb_allow") {
    if (!msg.params.count("usb_id") ||
        msg.params.find("usb_id")->second.empty()) {
      std::cout << mess_beg << mess_end;
      std::cerr << "bad request for usb allow,doing nothing" << std::endl;
      return true;
    }
    guard.AllowOrBlockDevice(msg.params.find("usb_id")->second, true);
    std::cout << mess_beg << "status" << WrapWithQuotes("OK") << mess_end;
    return true;
  }

  // block device with id
  if (msg.action == "read" && msg.objects == "usb_block") {
    if (!msg.params.count("usb_id") ||
        msg.params.find("usb_id")->second.empty()) {
      std::cout << mess_beg << mess_end;
      std::cerr << "bad request for usb allow,doing nothing" << std::endl;
      return true;
    }
    guard.AllowOrBlockDevice(msg.params.find("usb_id")->second, false);
    std::cout << mess_beg << "status" << WrapWithQuotes("OK") << mess_end;
    return true;
  }

  // get udev rules list
  if (msg.action == "list" && msg.objects == "check_config_udev"){
    std::cerr << "[Dispatcher] Check config" <<std::endl;
    std::string str=mess_beg;
    for(const auto& pair :guard.GetConfigStatus().udev_warnings){
      str+=ToLisp("label_udev_rules_filename",pair.first);
    }
    str+=mess_end;
    std::cout <<str;
    return true;
  }

  // empty response
  std::cout << "(\n)\n";
  return true;
}
