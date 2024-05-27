/* File: message_dispatcher.hpp

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

#pragma once

#include "lisp_message.hpp"
#include <functional>

using DispatchFunc = std::function<bool(const LispMessage &)>;

/**
 * @class MessageDispatcher
 * @brief Accepts messages (LispMessage) from MessageReader and perfoms
 * appropriate actions
 */
class MessageDispatcher {
public:
  /**
   * @brief Constructor for Message Dispatcher
   * @param function bool(*)(const LispMessage&) as dispatcher implementation
   */
  explicit MessageDispatcher(DispatchFunc) noexcept;

  /**
   * @brief Perfom an appropriate action for msg
   * @param msg LispMessage from MessageReader
   */
  bool Dispatch(const LispMessage &msg) const noexcept;

private:
  const DispatchFunc p_dispatcher_func_;
};
