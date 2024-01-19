#pragma once

#include "lispmessage.hpp"
#include "usb_device.hpp"
#include "types.hpp"

class MessageDispatcher{
    public:
        MessageDispatcher() =default;
        
        bool Dispatch(const LispMessage& msg);
};

std::vector<UsbDevice> fakeLibGetUsbList();
