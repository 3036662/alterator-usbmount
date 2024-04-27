#include "dispatcher_impl.hpp"
#include "common_utils.hpp"
#include "lisp_message.hpp"
#include "log.hpp"
#include "types.hpp"
#include "usb_mount.hpp"
#include <boost/json/parse.hpp>
#include <exception>
#include <iostream>
#include <utility>

namespace alterator::usbmount {

using common_utils::Log;

DispatcherImpl::DispatcherImpl(UsbMount &usbmount) : usbmount_(usbmount) {}

bool DispatcherImpl::Dispatch(const LispMessage &msg) const noexcept {
  Log::Debug() << msg;
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
  using namespace common_utils;
  std::cout << kMessBeg;
  std::cout << WrapWithQuotes(EscapeQuotes(usbmount_.getRulesJson()));
  std::cout << kMessEnd;
  return true;
}

bool DispatcherImpl::GetUsersGroups() const noexcept {
  using namespace common_utils;
  std::cout << kMessBeg;
  std::cout << WrapWithQuotes(EscapeQuotes(usbmount_.GetUsersGroups()));
  std::cout << kMessEnd;
  return true;
}

bool DispatcherImpl::SaveRules(const LispMessage &msg) const noexcept {
  using namespace common_utils;
  std::cout << kMessBeg;
  if (msg.params.count("data") > 0) {
    std::string res_str = usbmount_.SaveRules(msg.params.at("data"));
    try {
      auto res_js = boost::json::parse(res_str);
      std::cout << WrapWithQuotes(
          res_js.as_object().at("STATUS").as_string().c_str());
    } catch (const std::exception &ex) {
      std::cout << WrapWithQuotes("FAIL");
    }
  }
  std::cout << kMessEnd;
  return true;
}

} // namespace alterator::usbmount