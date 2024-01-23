#include "guard.hpp"
#include <iostream>

Guard::Guard(): ptr_ipc(nullptr)
{
    try{
        ptr_ipc=std::make_unique<usbguard::IPCClient>(true);
    }
    catch(usbguard::Exception& e){
        std::cerr << "Error connecting to USBGuard daemon \n"
        << e.what() << std::endl;
    }

}

bool Guard::HealthStatus(){
    return ptr_ipc && ptr_ipc->isConnected();
}


std::vector<UsbDevice> Guard::ListCurrentUsbDevices(){
    std::vector<UsbDevice> res;
    if (!HealthStatus()) return res;
    std::vector<usbguard::Rule> rules=ptr_ipc->listDevices(default_query);
    for (const usbguard::Rule& rule : rules) {
         res.emplace_back(
            rule.getRuleID(),
            rule.targetToString(rule.getTarget()),
            rule.getName(),
            rule.getDeviceID().toString(),
            rule.getViaPort(),
            rule.getWithConnectType()
         );
         //std::cerr << rule.getRuleID() << ": " << rule.toString() << std::endl;
    }
    return res;
}

// just in case
// this strings can be recieved from the rule, but they are not used yet
       // std::string status =rule.targetToString(rule.getTarget());
        // std::string device_id =rule.getDeviceID().toString();
        // std::string name=rule.getName();
        // std::string port = rule.getViaPort();
        // int rule_id=rule.getRuleID();
        // std::string conn_type=rule.getWithConnectType()
        //std::string serial = rule.getSerial();
        //std::string hash = rule.getHash();
        //std::string par_hash = rule.getParentHash();
        //std::string interface = rule.getWithConnectType();
        //std::string withInterf= rule.attributeWithInterface().toRuleString();
        //std::string cond=rule.attributeConditions().toRuleString();
        //
        // std::string label;
        // try{
        //    label =rule.getLabel();
        // }
        // catch(std::runtime_error& ex){
        //     std::cerr << ex.what();
        // }
        
        