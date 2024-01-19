#include "message_dispatcher.hpp"

bool MessageDispatcher::Dispatch(const LispMessage& msg){
        std::cerr << msg <<std::endl;
        return true;
}