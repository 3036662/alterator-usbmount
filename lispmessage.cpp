#include "lispmessage.hpp"

LispMessage::LispMessage() {}

LispMessage::LispMessage(const std::string& act,
                const std::string& obj,
                const std::unordered_map<std::string,std::string>& prms):
                action(act), objects(obj), params(prms){}

std::ostream& operator  << (std::ostream& os,const LispMessage& mes){
    os << "Action: " << mes.action <<std::endl
              << "Objects: " << mes.objects << std::endl;
    for (auto it = mes.params.cbegin(); it != mes.params.end(); ++it){
        os << it->first << " : " << it->second << std::endl;
    }
    return os;
}
