/* File: active_device.hpp

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

#include "serializable_for_lisp.hpp"
#include "types.hpp"
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <cstddef>
#include <string>

namespace alterator::usbmount {

namespace json = boost::json;

struct ActiveDevice : SerializableForLisp<ActiveDevice> {
  size_t index = 0;
  std::string block;
  std::string fs;
  std::string vid;
  std::string pid;
  std::string serial;
  std::string mount_point;
  std::string status;

  vecPairs SerializeForLisp() const noexcept;

  /**
   * @brief Construct a new Active Device object
   * @param obj
   * @throws std::invalid_argument
   */
  explicit ActiveDevice(const json::object &obj);
};

} // namespace alterator::usbmount