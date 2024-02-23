#include "message_reader.hpp"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "lispmessage.hpp"

MessageReader::MessageReader(guard::Guard &guard) : dispatcher(guard) {}

void MessageReader::Loop() {
  std::string line;
  bool msg_in_progress = false;
  std::string action;
  std::string objects;
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
    // end of messages
    else {
      if (!msg_in_progress)
        break;
    }

    // the end of a message
    if (boost::contains(line, "_message:end")) {
      msg_in_progress = false;
      if (!action.empty() && !objects.empty()) {
        LispMessage request_message(action, objects, params);
        dispatcher.Dispatch(request_message);
      }
      params.clear();
      action.clear();
      objects.clear();
      line.clear();
      continue;
    }

    // find action
    if (boost::starts_with(line, str_action)) {
      boost::erase_first(line, str_action);
      boost::trim(line);
      action = line;
      line.clear();
      continue;
    }

    // find object
    if (boost::starts_with(line, str_objects)) {
      boost::erase_first(line, str_objects);
      boost::trim(line);
      objects = line;
      line.clear();
      continue;
    }

    // find parameters
    size_t pos = line.find(':');
    if (pos != std::string::npos) {
      params.emplace(line.substr(0, pos), // param
                     line.substr(pos + 1) // value
      );
    }

    line.clear();
  }
}