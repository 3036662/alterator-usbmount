#pragma once
#include <string>

struct UsbUdevDevice {
  std::string action;
  std::string vid;
  std::string pid;
  std::string subsystem;
  std::string block_name;
};