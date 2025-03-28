/* File: dispatcher_impl.cpp

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

#include "dispatcher_impl.hpp"
#include "common_utils.hpp"
#include "lisp_message.hpp"
#include "log.hpp"
#include "log_reader.hpp"
#include "types.hpp"
#include "usb_mount.hpp"
#include <algorithm>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <exception>
#include <iostream>
#include <utility>

namespace alterator::usbmount {

using common_utils::Log;

DispatcherImpl::DispatcherImpl(UsbMount &usbmount) : usbmount_(usbmount) {}

bool DispatcherImpl::Dispatch(const LispMessage &msg) const noexcept {
  // Log::Debug() << msg;
  if (msg.action == "list" && msg.objects == "list_block") {
    return ListBlockDevices();
  }
  if (msg.action == "read" && msg.objects == "list-devices") {
    return ListRules();
  }

  if (msg.action == "read" && msg.objects == "get_users_groups") {
    return GetUsersGroups();
  }

  if (msg.action == "read" && msg.objects == "save_rules") {
    return SaveRules(msg);
  }

  if (msg.action == "read" && msg.objects == "health") {
    return Health();
  }
  if (msg.action == "read" && msg.objects == "run") {
    return RunDaemon();
  }
  if (msg.action == "read" && msg.objects == "stop") {
    return StopDaemon();
  }
  if (msg.action == "read" && msg.objects == "read_log") {
    return ReadLog(msg);
  }

  std::cout << kMessBeg << kMessEnd;
  return true;
}

bool DispatcherImpl::ListBlockDevices() const noexcept {
  auto devices = usbmount_.ListDevices();
  std::cout << kMessBeg;
  for (const auto &device : devices) {
    std::cout << common_utils::ToLisp(device);
  }
  std::cout << kMessEnd;
  return true;
}

bool DispatcherImpl::ListRules() const noexcept {
  namespace cu = common_utils;
  std::cout << kMessBeg;
  std::cout << cu::WrapWithQuotes(cu::EscapeQuotes(usbmount_.getRulesJson()));
  std::cout << kMessEnd;
  return true;
}

bool DispatcherImpl::GetUsersGroups() const noexcept {
  namespace cu = common_utils;
  std::cout << kMessBeg;
  std::cout << cu::WrapWithQuotes(cu::EscapeQuotes(usbmount_.GetUsersGroups()));
  std::cout << kMessEnd;
  return true;
}

bool DispatcherImpl::SaveRules(const LispMessage &msg) const noexcept {
  namespace cu = common_utils;
  std::cout << kMessBeg;
  if (msg.params.count("data") > 0) {
    const std::string res_str = usbmount_.SaveRules(msg.params.at("data"));
    try {
      auto res_js = boost::json::parse(res_str);
      std::cout << cu::WrapWithQuotes(
          res_js.as_object().at("STATUS").as_string().c_str());
    } catch (const std::exception &ex) {
      std::cout << cu::WrapWithQuotes("FAIL");
    }
  }
  std::cout << kMessEnd;
  return true;
}

bool DispatcherImpl::Health() const noexcept {
  std::cout << kMessBeg;
  std::cout << common_utils::WrapWithQuotes(
      (usbmount_.Health() ? "OK" : "DEAD"));
  std::cout << kMessEnd;
  return true;
}

bool DispatcherImpl::RunDaemon() const noexcept {
  std::cout << kMessBeg;
  std::cout << common_utils::WrapWithQuotes(usbmount_.Run() ? "OK" : "FAIL");
  std::cout << kMessEnd;
  return true;
}

bool DispatcherImpl::StopDaemon() const noexcept {
  std::cout << kMessBeg;
  std::cout << common_utils::WrapWithQuotes(usbmount_.Stop() ? "OK" : "FAIL");
  std::cout << kMessEnd;
  return true;
}

bool DispatcherImpl::ReadLog(const LispMessage &msg) noexcept {
  if (msg.params.count("page") == 0 || msg.params.count("filter") == 0) {
    Log::Error() << "Wrong parameters for log reading";
    return false;
  }
  std::cout << kMessBeg;
  try {
    const uint page_number =
        common_utils::StrToUint(msg.params.at("page")).value_or(0);
    uint per_page = 20;
    if (msg.params.count("per_page") > 0) {
      std::cerr << "per page value" << msg.params.at("per_page") << "\n";
      per_page = common_utils::StrToUint(msg.params.at("per_page")).value_or(5);
    }
    const std::string filter = msg.params.at("filter");
    const common_utils::LogReader reader("/var/log/alt-usb-automount/log.txt");
    auto res = reader.GetByPage({filter}, page_number, per_page);
    boost::json::object json_result;
    json_result["total_pages"] = res.pages_number;
    json_result["current_page"] = res.curr_page;
    json_result["data"] = boost::json::array();
    for (auto &str : res.data) {
      json_result["data"].as_array().emplace_back(
          common_utils::HtmlEscape(str));
    }
    std::cout << common_utils::WrapWithQuotes(
        common_utils::EscapeQuotes(boost::json::serialize(json_result)));
  } catch (const std::exception &ex) {
    Log::Error() << "[ReadLog] Error reading logs";
    Log::Error() << "[ReadLog] " << ex.what();
  }

  std::cout << kMessEnd;
  return true;
}

} // namespace alterator::usbmount