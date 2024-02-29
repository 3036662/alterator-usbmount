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
  explicit MessageReader(guard::Guard &guard) noexcept;

  /// @brief Main loop - reades messages and sens them to dispatcher
  void Loop() const noexcept;

private:
  MessageDispatcher dispatcher_;
  const std::string kStrAction = "action:";
  const std::string kStrObjects = "_objects:";
};

#endif // MESSAGE_READER_HPP
