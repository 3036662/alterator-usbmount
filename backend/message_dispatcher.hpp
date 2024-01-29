#pragma once

#include "guard.hpp"
#include "lispmessage.hpp"
#include "types.hpp"
#include "usb_device.hpp"

/**
 * @class MessageDispatcher
 * @brief Accepts messages (LispMessage) from MessageReader and perfoms
 * appropriate actions
 */
class MessageDispatcher {
public:
  /**
   * @brief Constructor for Message Dispatcher
   * @param guard The Guard object
   */
  MessageDispatcher(Guard &guard);
  /**
   * @brief Perfom an appropriate action for msg
   * @param msg LispMessage from MessageReader
   */
  bool Dispatch(const LispMessage &msg);

private:
  Guard &guard;
  const std::string mess_beg = "(";
  const std::string mess_end = ")";
};
