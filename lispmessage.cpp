#include "lispmessage.hpp"

LispMessage::LispMessage() {}

std::ostream& operator  << (std::ostream& os,const LispMessage& mes){
    os << "Action: " << mes.action <<std::endl
              << "Objects: " << mes.objects;
    for (auto it = mes.params.cbegin(); it != mes.params.end(); ++it){
        os << it->first << " : " << it->second << std::endl;
    }
    return os;
}
