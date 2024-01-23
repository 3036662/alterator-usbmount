#pragma once
#include <string>
#include <vector>
#include "types.hpp"
#include "usb_device.hpp"

// vec of pairs to lisp string
std::string ToLisp(const vecPairs& vec);

// wrap with escaped quotes
std::string WrapWithQuotes(const std::string& str);

// data mocking
std::vector<UsbDevice> fakeLibGetUsbList();