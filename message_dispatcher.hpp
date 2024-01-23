#pragma once

#include "lispmessage.hpp"
#include "usb_device.hpp"
#include "types.hpp"
#include "guard.hpp"

class MessageDispatcher{
    public:
        MessageDispatcher(Guard& guard);
        bool Dispatch(const LispMessage& msg);

    private:
        Guard& guard;
        const std::string mess_beg="(";
        const std::string mess_end=")";
};


