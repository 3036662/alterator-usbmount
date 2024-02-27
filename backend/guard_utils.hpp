#pragma once

/**
 * @brief Usb-related free-standing functions
 *
 */

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace guard::utils {

///@brief fold list of interface { 03:01:02 03:01:01 } to [03:*:*]
///@param i_type string with list of interfaces from usbguard
std::vector<std::string> FoldUsbInterfacesList(std::string i_type);

/**
 * @brief Creates map vendor ID : vendor Name with in one pass to fil
 * @param vendors set of vendor IDs
 * @return std::map<std::string,std::string> Vendor ID : Vendor Name
 */
std::unordered_map<std::string, std::string>
MapVendorCodesToNames(const std::unordered_set<std::string> &vendors) noexcept;

} // namespace guard::utils