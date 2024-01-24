#pragma once

#include "guard.hpp"
#include "lispmessage.hpp"
#include "types.hpp"
#include "usb_device.hpp"

class MessageDispatcher {
public:
  MessageDispatcher(Guard &guard);
  bool Dispatch(const LispMessage &msg);

private:
  Guard &guard;
  const std::string mess_beg = "(";
  const std::string mess_end = ")";
};
