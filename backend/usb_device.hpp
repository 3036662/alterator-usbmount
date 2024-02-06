#pragma once
#include "serializible_for_lisp.hpp"
#include "types.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace guard {

/**
 * @class UsbDevice
 * @brief Represents a usb device
 */

class UsbDevice : public SerializableForLisp<UsbDevice> {
public:
  int number;
  std::string status;
  std::string name;
  std::string vid;
  std::string pid;
  std::string port;
  std::string connection;
  std::string i_type;

  /// @brief Constructor for a UsbDevice
  /// @param num Just a number to display in the frontend
  /// @param status_ Current status (allowed or blocked)
  /// @param name_  A name of device
  /// @param id_  An id for device (string)
  /// @param port_ Device port
  /// @param connection_ Connection type (for instance - hot-plug)
  /// @param i_type_ Interface type
  UsbDevice(int num, const std::string &status_, const std::string &name_,
            const std::string &vid_, const std::string &pid_,
            const std::string &port_, const std::string &connection_,
            const std::string &i_type_);

  UsbDevice(const UsbDevice &) = default;

  /**
   * @brief Serialize data to vector of pairs
   * html_name : value for displaing in frontend
   * @return Vector of string pairs, suitable for alterator
   * frontend
   */
  vecPairs SerializeForLisp() const;
};

/**
 * @class UsbType
 * @brief Represents USB class code
 * @details Information that is used to identify a device’s functionality
 * The information is contained in three bytes
 * with the names Base Class, SubClass, and Protocol. (
 */
class UsbType {
public:
  char base;
  char sub;
  char protocol;
  std::string base_str;
  std::string sub_str;
  std::string protocol_str;

  /// @brief Constructor from string
  /// @param string Fotmatted string 00:00:00
  /// @throws std::invalid_argument  std::out_of_range std::logical_error
  UsbType(const std::string &string);

  UsbType(UsbType &&) noexcept = default;
  UsbType &operator=(UsbType &&) noexcept = default;
};

} // namespace guard