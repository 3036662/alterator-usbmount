#pragma once
#include "types.hpp"
#include "usb_device.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>


/// @brief Wrap string with esape coutes
/// @param str String to wrap
/// @return New wrapped string
std::string WrapWithQuotes(const std::string &str);

/// @brief Mock usb list just for develop and testing purposes
/// @return Mocked vector of Usbdevice
std::vector<UsbDevice> fakeLibGetUsbList();

/// @brief Exception-safe string to uint32_t conversion
/// @param id String, containing number
/// @return Numerical value (uint32_t)
uint32_t StrToUint(const std::string &str) noexcept;

/// @brief Recursive search for all files in direcrory
/// @param dir Directory to search in
/// @param ext Extension - which files to search
/// @return Vector of string pathes for all files in directory (recursive)
/// @warning may throw exceptions
std::vector<std::string>
FindAllFilesInDirRecursive(const std::string &dir,
                           const std::string &ext = std::string());


/**
* @brief Converts vec of string pairs to a LispSring
* @param vec Vector of string
* @return String, suitable for sending to Lisp(Alterator)
* @details  Can be used map html table labels to data values
*/
template <typename T>
std::string ToLisp(const SerializableForLisp<T>& obj){
  std::string res;
  vecPairs vec {obj.SerializeForLisp()};
  res += "(";
  // ignore firs name, use only value
  auto it = vec.cbegin();
  if (it!=vec.cend()){
    res+=WrapWithQuotes(it->second);
    res+=" ";
    ++it;
  }
  // use name:value
  while (it!=vec.cend()){
       res += WrapWithQuotes(it->first);
       res += " ";
       res += WrapWithQuotes(it->second);
       res += " ";
       ++it;
  }
  res += ")";
  // std::cerr << "result string: " <<std::endl <<res << std::endl;
  return res;
}

/**
 * @brief Maps one name:value -> lisp string (name , value) for html tables
 * @param name Html label name
 * @param value Value 
 * @details Name parameter will be ignored - table with one column
 * needs only value
*/ 
std::string ToLisp([[maybe_unused]] const std::string& name,const std::string& value);