/* File: lisp_message.hpp

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

#ifndef LISP_MESSAGE_HPP
#define LISP_MESSAGE_HPP

#include <iostream>
#include <string>
#include <unordered_map>

/**
 * @class LispMessage
 * @brief Represents a message from alterator frontend
 */
struct LispMessage {
  struct MsgAction {
    std::string val;
  };
  struct MsgObject {
    std::string val;
  };
  std::string action;
  std::string objects;
  std::unordered_map<std::string, std::string> params;
  /// @brief Constructor for an empty message
  LispMessage();
  /// @brief Constructor for full-fledged message
  /// @param act String action (read,list,etc.)
  /// @param obj Alterator path to backend
  /// @param prms A map of string pairs repres. parameters
  LispMessage(
      const MsgAction &act, const MsgObject &obj,
      const std::unordered_map<std::string, std::string> &prms) noexcept;
};

/// @brief  The output operator for the stream
/// @param os Stream
/// @param mes LispMessage object
/// @return Stream
std::ostream &operator<<(std::ostream &ostream, const LispMessage &mes);

#endif // LISP_MESSAGE_HPP
