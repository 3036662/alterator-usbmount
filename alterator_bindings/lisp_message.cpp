/* File: lisp_message.cpp

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

#include "lisp_message.hpp"

LispMessage::LispMessage() {}

LispMessage::LispMessage(
    const MsgAction &act, const MsgObject &obj,
    const std::unordered_map<std::string, std::string> &prms) noexcept
    : action(act.val), objects(obj.val), params(prms) {}

std::ostream &operator<<(std::ostream &ostream, const LispMessage &mes) {
  ostream << "Action: " << mes.action << "\n"
          << "Objects: " << mes.objects << "\n";
  for (auto it = mes.params.cbegin(); it != mes.params.end(); ++it) {
    ostream << it->first << " : " << it->second << "\n";
  }
  return ostream;
}
