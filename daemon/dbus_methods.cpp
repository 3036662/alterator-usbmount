#include "dbus_methods.hpp"
#include "dal/dto.hpp"
#include "dal/local_storage.hpp"
#include "udev_monitor.hpp"
#include "usb_udev_device.hpp"
#include "utils.hpp"
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <exception>
#include <iterator>
#include <sdbus-c++/Message.h>
#include <string>
#include <sys/socket.h>
#include <utility>

namespace usbmount {

namespace json = boost::json;

DbusMethods::DbusMethods(std::shared_ptr<UdevMonitor> udev_monitor,
                         std::shared_ptr<spdlog::logger> logger)
    : connection_(sdbus::createSystemBusConnection(service_name)),
      dbus_object_ptr(sdbus::createObject(*connection_, object_path)),
      logger_(std::move(logger)), dbase_(dal::LocalStorage::GetStorage()),
      udev_monitor_(std::move(udev_monitor)) {
  // Health()
  dbus_object_ptr->registerMethod(interface_name, "health", "", "s", Health);
  // ListDevices()
  dbus_object_ptr->registerMethod(
      interface_name, "ListDevices", "", "s",
      [this](const sdbus::MethodCall &call) { ListActiveDevices(call); });
  // ListRules
  dbus_object_ptr->registerMethod(
      interface_name, "ListRules", "", "s",
      [this](const sdbus::MethodCall &call) { ListActiveRules(call); });
  // Get users and groups
  dbus_object_ptr->registerMethod(
      interface_name, "GetUsersAndGroups", "", "s",
      [this](const sdbus::MethodCall &call) { GetSystemUsersAndGroups(call); });
  // CanAnotherUserUnmount("/dev/sd..")
  dbus_object_ptr->registerMethod(interface_name, "CanAnotherUserUnmount", "s",
                                  "s", [this](sdbus::MethodCall call) {
                                    CanAnotherUserUnmount(std::move(call));
                                  });
  // CanUserMount("/dev/sd..")
  dbus_object_ptr->registerMethod(
      interface_name, "CanUserMount", "s", "s",
      [this](sdbus::MethodCall call) { CanUserMount(std::move(call)); });
  dbus_object_ptr->finishRegistration();
}

void DbusMethods::Run() { connection_->enterEventLoopAsync(); }

void DbusMethods::Health(const sdbus::MethodCall &call) {
  auto reply = call.createReply();
  reply << "OK";
  reply.send();
}

void DbusMethods::CanAnotherUserUnmount(sdbus::MethodCall call) {
  std::string dev;
  call >> dev;
  logger_->debug("Polkit request for device (CanUserUnMount)" + dev);
  auto reply = call.createReply();
  // if this device was mounted by this program - reply YES
  try {
    auto index = dbase_->mount_points.Find(dev);
    if (index) {
      reply << "YES";
      logger_->debug("Daemon response to polkit = YES");
    } else {
      reply << "UNKNOWN_MOUNTPOINT";
      logger_->debug("Daemon response to polkit = UNKNOWN_MOUNTPOINT");
    }
  } catch (const std::exception &ex) {
    logger_->error("Dbus::CanUserUnmount Can't query the dbase for device + {}",
                   dev);
  }
  logger_->flush();
  reply.send();
}

void DbusMethods::CanUserMount(sdbus::MethodCall call) {
  std::string dev;
  call >> dev;
  /*
  Read local db define is this user is allowed to mount this drive
  (if drive is in db - reply NO - automount will mount it )
  */
  logger_->debug("Polkit request for device (CanUserMount)" + dev);
  sdbus::MethodReply reply = call.createReply();
  try {
    UsbUdevDevice device({dev, "add"});
    dal::Device dto_device({device.vid(), device.pid(), device.serial()});
    if (dbase_->permissions.Find(dto_device)) {
      logger_->debug("daemon reply to polkit = NO");
      reply << "NO";
    } else {
      logger_->debug("daemon reply to polkit = YES");
      reply << "YES";
    }
    logger_->flush();
    reply.send();
    return;
  } catch (const std::exception &ex) {
    logger_->error("[Dbus::CanUserMount]Can't construct udev device from " +
                   dev);
  }
  // just in case  -must be unreacheable
  reply << "YES";
  reply.send();
}

void DbusMethods::ListActiveDevices(const sdbus::MethodCall &call) {
  logger_->debug("[DBUS][ListActiveDevices]");
  auto devices = udev_monitor_->GetConnectedDevices();
  json::array response_array;
  for (const auto &dev : devices) {
    json::object obj;
    obj["device"] = dev.block_name();
    obj["vid"] = dev.vid();
    obj["pid"] = dev.pid();
    obj["serial"] = dev.serial();
    obj["fs"] = dev.filesystem();
    // mount_point
    auto index_mount = dbase_->mount_points.Find(dev.block_name());
    if (index_mount)
      obj["mount"] = dbase_->mount_points.Read(*index_mount).mount_point();
    else
      obj["mount"] = "";
    // permissions
    auto perm_index = dbase_->permissions.Find(
        dal::Device({dev.vid(), dev.pid(), dev.serial()}));
    perm_index.has_value() ? obj["status"] = "owned" : obj["status"] = "free";
    response_array.emplace_back(std::move(obj));
  }
  sdbus::MethodReply reply = call.createReply();
  // logger_->debug("[DBUS] Response = {}", json::serialize(response_array));
  reply << json::serialize(response_array);
  reply.send();
}

void DbusMethods::ListActiveRules(const sdbus::MethodCall &call) {
  logger_->debug("[DBUS][ListActiveRules]");
  json::array response_array;
  auto rules = dbase_->permissions.getAll();
  for (auto &rule : rules) {
    json::object obj;
    obj["id"] = std::to_string(rule.first);
    obj["perm"] = rule.second->ToJson();
    response_array.emplace_back(std::move(obj));
  }
  sdbus::MethodReply reply = call.createReply();
  reply << json::serialize(response_array);
  reply.send();
}

void DbusMethods::GetSystemUsersAndGroups(const sdbus::MethodCall &call) {
  logger_->debug("[DBUS][GetUsersAndGroups]");
  json::object res;
  const auto id_limits = utils::GetSystemUidMinMax(logger_);
  if (id_limits.has_value()) {
    auto users = utils::GetHumanUsers(id_limits.value(), logger_);
    if (!std::empty(users)) {
      res["users"] = json::array();
      for (const auto &usr : users)
        res["users"].as_array().emplace_back(usr.ToJson());
    }
    auto groups = utils::GetHumanGroups(id_limits.value(), logger_);
    if (!groups.empty()) {
      res["groups"] = json::array();
      for (const auto &grp : groups)
        res["groups"].as_array().emplace_back(grp.ToJson());
    }
  }
  sdbus::MethodReply reply = call.createReply();
  reply << json::serialize(res);
  reply.send();
}

} // namespace usbmount