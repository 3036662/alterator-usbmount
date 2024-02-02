#pragma once

#include <memory>
#include <optional>
#include <sdbus-c++/sdbus-c++.h> 

namespace dbus_buindings{

/**
 * @class Systemd
 * @brief A binding for some usefull systemd DBus functions
 * 
*/
class Systemd{
public:
    Systemd(); /// constructor - creates connection to DBus
    
    /**
     * @brief Check whether unit.service is enabled
     * @param unit_name File.service (string)
     * @return True if unit is enabled,False if not, nothing if error(optional)
     * */ 
    std::optional<bool> IsUnitEnabled(const std::string &unit_name) noexcept;
    
    /**
     * @brief Check whether unit.service is active
     * @param unit_name File.service active
     * @returnTrue if unit is enabled,False if not, nothing if error (optional)
     * */ 
    std::optional<bool> IsUnitActive(const std::string &unit_name) noexcept;

private:
 const std::string destinationName = "org.freedesktop.systemd1";
   const std::string objectPath = "/org/freedesktop/systemd1";
   const std::string systemd_interface_manager ="org.freedesktop.systemd1.Manager";
   const std::string systemd_interface_unit = "org.freedesktop.systemd1.Unit";
   std::unique_ptr<sdbus::IConnection> connection; 
   void ConnectToSystemDbus() noexcept;
   bool Health(); /// check if normally connectted

   /**
    * @brief Create proxy for systemd particular path
    * @param path String interface (for inst. /org/freedesktop/systemd1)'
    * @return Unique ptr to IProxy object
    * */ 
   std::unique_ptr<sdbus::IProxy> CreateProxyToSystemd (const std::string &path);

   
   

  
};

} // dbus buindings