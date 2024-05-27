/* File: log.hpp

  Copyright (C)   2024
  Author: Oleg Proskurin, <proskurinov@basealt.ru>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <https://www.gnu.org/licenses/>.

*/

#pragma once
#include <iostream>
namespace common_utils {

class Log {
public:
  class Debug {
  public:
    template <typename T> const Debug &operator<<(const T &val) const noexcept {
      std::cerr << val;
      return *this;
    }

    Debug() noexcept { std::cerr << "[DEBUG] "; }
    ~Debug() { std::cerr << "\n"; }
  };

  class Info {
  public:
    template <typename T> const Info &operator<<(const T &val) const noexcept {
      std::cerr << val;
      return *this;
    }
    Info() noexcept { std::cerr << "[INFO] "; }
    ~Info() { std::cerr << "\n"; }
  };
  class Warning {
  public:
    template <typename T>
    const Warning &operator<<(const T &val) const noexcept {
      std::cerr << val;
      return *this;
    }
    Warning() noexcept { std::cerr << "[WARNING] "; }
    ~Warning() { std::cerr << "\n"; }
  };
  class Error {
  public:
    template <typename T> const Error &operator<<(const T &val) const noexcept {
      std::cerr << val;
      return *this;
    }
    Error() noexcept { std::cerr << "[ERROR] "; }
    ~Error() { std::cerr << "\n"; }
  };

  class Test {
  public:
    template <typename T> const Test &operator<<(const T &val) const noexcept {
      std::cerr << val;
      return *this;
    }
    Test() noexcept { std::cerr << "[TEST] "; }
    ~Test() { std::cerr << "\n"; }
  };
};

} // namespace common_utils