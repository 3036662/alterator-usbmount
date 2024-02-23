#include "lispmessage.hpp"

LispMessage::LispMessage() {}

LispMessage::LispMessage(
    const std::string &act, const std::string &obj,
    const std::unordered_map<std::string, std::string> &prms)
    : action(act), objects(obj), params(prms) {}

std::ostream &operator<<(std::ostream &ostream, const LispMessage &mes) {
  ostream << "Action: " << mes.action << "\n"
     << "Objects: " << mes.objects << "\n";
  for (auto it = mes.params.cbegin(); it != mes.params.end(); ++it) {
    ostream << it->first << " : " << it->second << "\n";
  }
  return ostream;
}
