#pragma once
#include "types.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "serializible_for_lisp.hpp"

/**
 * @class UsbDevice
 * @brief Represents and usb device
 */

class UsbDevice : public SerializableForLisp<UsbDevice> {
public:
  int number;
  std::string status;
  std::string name;
  std::string id;
  std::string port;
  std::string connection;

  /// @brief Constructor for a UsbDevice
  /// @param num Just a number to display in the frontend
  /// @param status_ Current status (allowed or blocked)
  /// @param name_  A name of device
  /// @param id_  An id for device (string)
  /// @param port_ Device port
  /// @param connection_ Connection type (for instance - hot-plug)
  UsbDevice(int num, const std::string &status_, const std::string &name_,
            const std::string &id_, const std::string &port_,
            const std::string &connection_)
      : number(num), status(status_), name(name_), id(id_), port(port_),
        connection(connection_) {}

  /**
   * @brief Serialize data to vector of pairs
   * html_name : value for displaing in frontend
   * @return Vector of string pairs, suitable for alterator
   * frontend
   */
  vecPairs SerializeForLisp() const;
};
