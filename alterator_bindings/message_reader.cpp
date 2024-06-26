/* File: message_reader.cpp

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

#include "message_reader.hpp"
#include "lisp_message.hpp"
#include "message_dispatcher.hpp"
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

// MessageReader::MessageReader(guard::Guard &guard) noexcept
//     : dispatcher_(guard) {}

MessageReader::MessageReader(DispatchFunc dispatcher_func) noexcept
    : dispatcher_(std::move(dispatcher_func)) {}

void MessageReader::Loop() const noexcept {
  std::string line;
  bool msg_in_progress = false;
  LispMessage::MsgAction action;
  LispMessage::MsgObject objects;
  std::unordered_map<std::string, std::string> params;
  // read stdin loop
  while (std::getline(std::cin, line)) {
    boost::trim(line);
    // message begin
    if (boost::contains(line, "_message:begin")) {
      msg_in_progress = true;
      line.clear();
      continue;
    }
    // end of loop
    if (!boost::contains(line, "_message:begin") && !msg_in_progress) {
      break;
    }
    // the end of a message
    if (boost::contains(line, "_message:end")) {
      msg_in_progress = false;
      if (!action.val.empty() && !objects.val.empty()) {
        const LispMessage request_message(action, objects, params);
        dispatcher_.Dispatch(request_message);
      }
      params.clear();
      action.val.clear();
      objects.val.clear();
      line.clear();
      continue;
    }
    // find action
    if (boost::starts_with(line, kStrAction)) {
      boost::erase_first(line, kStrAction);
      boost::trim(line);
      action.val = line;
      line.clear();
      continue;
    }
    // find object
    if (boost::starts_with(line, kStrObjects)) {
      boost::erase_first(line, kStrObjects);
      boost::trim(line);
      objects.val = line;
      line.clear();
      continue;
    }
    // find parameters
    const size_t pos = line.find(':');
    if (pos != std::string::npos) {
      params.emplace(line.substr(0, pos), // param
                     line.substr(pos + 1) // value
      );
    }
    line.clear();
  }
}