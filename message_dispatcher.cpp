#include "message_dispatcher.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/algorithm.hpp>
#include "utils.hpp"

bool MessageDispatcher::Dispatch(const LispMessage& msg){
    std::cerr << msg <<std::endl;
    std::cerr<<"curr action "<< msg.action <<std::endl;
    if (msg.action=="list"){
        std::cerr <<"dispatcher->list"<<std::endl;
        if (msg.params.count("list") && 
            msg.params.find("list")->second=="current_usb_list"){
               // TODO 
             std::vector<UsbDevice> vec_usb=fakeLibGetUsbList();
             for (const auto& usb : vec_usb){
               // std::cout << ToLisp(usb.SerializeForLisp());
             }  
        }
    }

    return true;
}

std::vector<UsbDevice> fakeLibGetUsbList(){
    std::vector<UsbDevice> res;
    for (int i=0; i<10; ++i){
        std::string str_num=std::to_string(i);
        res.emplace_back(1,
                        "allowed",
                        "name"+str_num,
                        "id"+str_num,
                        "port"+str_num,
                        "conn"+str_num);
    }
    return res;
}