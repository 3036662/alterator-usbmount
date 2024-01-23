#include "message_dispatcher.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/algorithm.hpp>
#include "utils.hpp"

MessageDispatcher::MessageDispatcher(Guard & guard):guard(guard){}

bool MessageDispatcher::Dispatch(const LispMessage& msg){
    std::cerr << msg <<std::endl;
    std::cerr<<"curr action "<< msg.action <<std::endl;
    
    // list usbs
    if (msg.action=="list" && msg.objects=="list_curr_usbs"){
        std::cerr <<"dispatcher->list"<<std::endl;
           //  std::vector<UsbDevice> vec_usb=fakeLibGetUsbList();
             std::vector<UsbDevice> vec_usb=guard.ListCurrentUsbDevices();
             std::cout <<mess_beg;
             for (const auto& usb : vec_usb){
                std::cout << ToLisp(usb.SerializeForLisp());
             }  
             std::cout << mess_end;
             return true;
    }

    // empty response
    std::cout <<"(\n)\n";

    return true;
}

