#pragma once
#include <USBGuard.hpp>
#include <IPCClient.hpp>
#include <memory>
#include "usb_device.hpp"


class Guard{
public:
    // guard can be constructed even if daemon in not active
    Guard();

    std::vector<UsbDevice> ListCurrentUsbDevices();

    bool AllowOrBlockDevice(std::string id,bool allow=false,bool permanent = true);

private:
   const std::string default_query="match"; 
   std::unique_ptr<usbguard::IPCClient> ptr_ipc; 
   
   // check if daemon is active
   bool HealthStatus() const ; 
};