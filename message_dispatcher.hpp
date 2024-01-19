#pragma once

#include "lispmessage.hpp"

class MessageDispatcher{
    public:
        MessageDispatcher() =default;
        
        bool Dispatch(const LispMessage& msg);
};
