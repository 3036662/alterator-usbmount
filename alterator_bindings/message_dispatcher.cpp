/* File: message_dispatcher.cpp

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

#include "message_dispatcher.hpp"
#include "common_utils.hpp"
#include "log.hpp"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <utility>

using namespace common_utils;

MessageDispatcher::MessageDispatcher(DispatchFunc pfunc) noexcept
    : p_dispatcher_func_(std::move(pfunc)) {}

bool MessageDispatcher::Dispatch(const LispMessage &msg) const noexcept {
  if (p_dispatcher_func_ == nullptr)
    return false;
  return p_dispatcher_func_(msg);
}
