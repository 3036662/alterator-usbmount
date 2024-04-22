#include "lisp_message.hpp"
#include "message_dispatcher.hpp"
#include "usb_mount.hpp"

namespace alterator::usbmount {

class DispatcherImpl {
public:
  explicit DispatcherImpl(UsbMount &guard);

  bool Dispatch(const LispMessage &msg) const noexcept;

private:
  bool ListBlockDevices() const noexcept;
  bool ListRules() const noexcept;
  bool GetUsersGroups() const noexcept;

  static constexpr const char *kMessBeg = "(";
  static constexpr const char *kMessEnd = ")";

  UsbMount &usbmount_;
};

} // namespace alterator::usbmount
