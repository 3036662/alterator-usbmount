#pragma once
#include "types.hpp"
#include "usb_device.hpp"
#include <cstdint>
#include <string>
#include <vector>

/// @brief Converts vec of string pairs to a LispSring
/// @param vec Vector of string
/// @return String, suitable for sending to Lisp(Alterator)
std::string ToLisp(const vecPairs &vec);

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