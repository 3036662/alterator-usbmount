/* File: message_reader.hpp

  Copyright (C)   2024
  Author: Oleg Proskurin, <proskurinov@basealt.ru>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <https://www.gnu.org/licenses/>.

*/

#ifndef MESSAGE_READER_HPP
#define MESSAGE_READER_HPP

#include "lisp_message.hpp"
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
  /**
   * @brief Construct a new Message Reader object
   * @param Function bool(*)(const LispMessage&) as dispatcher implementation
   */
  MessageReader(DispatchFunc) noexcept;

  /// @brief Main loop - reades messages and sens them to dispatcher
  void Loop() const noexcept;

private:
  MessageDispatcher dispatcher_;
  const std::string kStrAction = "action:";
  const std::string kStrObjects = "_objects:";
};

#endif // MESSAGE_READER_HPP
