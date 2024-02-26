#pragma once
#include "types.hpp"
#include "usb_device.hpp"
#include <boost/algorithm/string.hpp>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/// @brief Wrap string with esape coutes
/// @param str String to wrap
/// @return New wrapped string
std::string WrapWithQuotes(const std::string &str);

std::string QuoteIfNotQuoted(const std::string &str);

/// @brief Mock usb list just for develop and testing purposes
/// @return Mocked vector of Usbdevice
std::vector<guard::UsbDevice> fakeLibGetUsbList();

/// @brief Exception-safe string to uint32_t conversion
/// @param id String, containing number
/// @return Numerical value (uint32_t)
std::optional<uint32_t> StrToUint(const std::string &str) noexcept;

/// @brief Recursive search for all files in direcrory
/// @param dir Directory to search in
/// @param ext Extension - which files to search
/// @return Vector of string pathes for all files in directory (recursive)
/// @warning may throw exceptions
std::vector<std::string>
FindAllFilesInDirRecursive(const std::string &dir,
                           const std::string &ext = std::string()) noexcept;

/**
 * @brief Converts vec of string pairs to a LispSring
 * @param vec Vector of string
 * @return String, suitable for sending to Lisp(Alterator)
 * @details  Can be used map html table labels to data values.
 * Returns a lisp strig ("value1" "label2" "value2" ...)
 */
template <typename T> std::string ToLisp(const SerializableForLisp<T> &obj) {
  std::string res;
  vecPairs vec{obj.SerializeForLisp()};
  res += "(";
  // ignore firs name, use only value
  auto iter = vec.cbegin();
  if (iter != vec.cend()) {
    res += WrapWithQuotes(iter->second);
    res += " ";
    ++iter;
  }
  // use name:value
  while (iter != vec.cend()) {
    res += WrapWithQuotes(iter->first);
    res += " ";
    res += WrapWithQuotes(iter->second);
    res += " ";
    ++iter;
  }
  res += ")";
  // std::cerr << "result string: " <<std::endl <<res << std::endl;
  return res;
}

/**
 * @brief Constructs a lisp associative list from vector ov sring pairs
 * @param obj Any SerializableForLisp object
 * @return Lisp string - ((name "value")(name2 "value2"))
 */
template <typename T>
std::string ToLispAssoc(const SerializableForLisp<T> &obj) {
  std::string res;
  vecPairs vec{obj.SerializeForLisp()};
  res += '(';
  for (const auto &pair : vec) {
    res += '(';
    res += pair.first;
    res += ' ';
    res += WrapWithQuotes(pair.second);
    res += ')';
  }
  res += ')';
  return res;
}

/**
 * @brief Maps one name:value -> lisp string (name , value) for html tables
 * @param name Html label name
 * @param value Value
 * @details Name parameter will be ignored - table with one column
 * needs only value. Returns a lisp string ("value")
 */
std::string ToLisp([[maybe_unused]] const std::string &name,
                   const std::string &value);

/// @brief  Escapes double-quotes with slashes
/// @param str Source string
/// @return Escaped string
std::string EscapeQuotes(const std::string &str);

std::string UnQuote(const std::string &str);

/**
 * @brief Parse json string ["1","2","n"] to vector of int
 *
 * @param json string ["1","2","n"]
 * @return std::vector<uint>
 */
std::vector<uint> ParseJsonIntArray(std::string json) noexcept;

template <class result_t = std::chrono::milliseconds,
          class clock_t = std::chrono::steady_clock,
          class duration_t = std::chrono::milliseconds>
auto since(std::chrono::time_point<clock_t, duration_t> const &start) {
  return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}
