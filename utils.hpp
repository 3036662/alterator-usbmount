#pragma once
#include "types.hpp"
#include "usb_device.hpp"
#include <cstdint>
#include <string>
#include <vector>

// vec of pairs to lisp string
std::string ToLisp(const vecPairs &vec);

// wrap with escaped quotes
std::string WrapWithQuotes(const std::string &str);

// data mocking
std::vector<UsbDevice> fakeLibGetUsbList();

uint32_t StrToUint(const std::string &id) noexcept;