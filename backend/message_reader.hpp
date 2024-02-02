#ifndef MESSAGE_READER_HPP
#define MESSAGE_READER_HPP

#include "guard.hpp"
#include "message_dispatcher.hpp"
#include <string>

/**
 * @class MessageReader
 * @brief Reads message from alterator frontend
 *
 * Creates LispMessage objects ans sends them to MessageDispatcher
 */
class MessageReader {
public:
  /// @brief  Constructor
  /// @param guard Guard object
  MessageReader(guard::Guard &guard);

  /// @brief Main loop - reades messages and sens them to dispatcher
  void Loop();

private:
  MessageDispatcher dispatcher;

  const std::string str_action = "action:";
  const std::string str_objects = "_objects:";
};

#endif // MESSAGE_READER_HPP
