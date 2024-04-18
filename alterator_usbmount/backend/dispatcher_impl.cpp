#include "dispatcher_impl.hpp"
#include "common_utils.hpp"
#include "lisp_message.hpp"
#include "log.hpp"
#include "usb_mount.hpp"

namespace alterator::usbmount {

using common_utils::Log;

DispatcherImpl::DispatcherImpl(UsbMount &usbmount) : usbmount_(usbmount) {}

bool DispatcherImpl::Dispatch(const LispMessage &msg) const noexcept {
  Log::Debug() << msg;
  if (msg.action == "list" && msg.objects == "list_block") {
    return ListBlockDevices();
  }
  std::cout << kMessBeg << kMessEnd;
  return true;
}

bool DispatcherImpl::ListBlockDevices() const noexcept {
  auto devices = usbmount_.ListDevices();
  std::cout << kMessBeg;
  for (const auto &device : devices) {
    std::cout << common_utils::ToLisp(device);
  }
  std::cout << kMessEnd;
  return true;
}

} // namespace alterator::usbmount