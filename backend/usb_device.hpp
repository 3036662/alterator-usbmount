#pragma once
#include "types.hpp"
#include <iostream>
#include <string>
#include <vector>

class UsbDevice {
public:
  int number;
  std::string status;
  std::string name;
  std::string id;
  std::string port;
  std::string connection;

  UsbDevice(int num, const std::string &status_, const std::string &name_,
            const std::string &id_, const std::string &port_,
            const std::string &connection_)
      : number(num), status(status_), name(name_), id(id_), port(port_),
        connection(connection_) {}

  // vector of pairs name->value
  vecPairs SerializeForLisp() const;
};
