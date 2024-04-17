#include "lisp_message.hpp"
#include "message_dispatcher.hpp"
#include "usb_mount.hpp"

namespace guard {

class DispatcherImpl {
public:
  explicit DispatcherImpl(Guard &guard);

  bool Dispatch(const LispMessage &msg) const noexcept;

private:


  static constexpr const char *kMessBeg = "(";
  static constexpr const char *kMessEnd = ")";

   //&guard_;
};

} // namespace guard
