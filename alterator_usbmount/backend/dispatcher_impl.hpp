/* File: dispatcher_impl.hpp

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

#include "lisp_message.hpp"
#include "message_dispatcher.hpp"
#include "usb_mount.hpp"

namespace alterator::usbmount {

class DispatcherImpl {
public:
  explicit DispatcherImpl(UsbMount &);

  bool Dispatch(const LispMessage &msg) const noexcept;

private:
  bool ListBlockDevices() const noexcept;
  bool ListRules() const noexcept;
  bool GetUsersGroups() const noexcept;
  bool SaveRules(const LispMessage &) const noexcept;
  bool Health() const noexcept;
  bool RunDaemon() const noexcept;
  bool StopDaemon() const noexcept;
  static bool ReadLog(const LispMessage &) noexcept;

  static constexpr const char *kMessBeg = "(";
  static constexpr const char *kMessEnd = ")";

  UsbMount &usbmount_;
};

} // namespace alterator::usbmount
