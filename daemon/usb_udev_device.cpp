#include "usb_udev_device.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace usbmount {

UsbUdevDevice::UsbUdevDevice(
    std::unique_ptr<udev_device, decltype(&udev_device_unref)> &&device) {
  // action
  const char *p_action = udev_device_get_property_value(device.get(), "ACTION");
  SetAction(p_action);
  // subsystem
  const char *p_subsystem =
      udev_device_get_property_value(device.get(), "SUBSYSTEM");
  if (p_subsystem != NULL)
    subsystem_ = p_subsystem;
  if (subsystem_ != "block")
    throw std::logic_error("wrong subsystem");
  // subsystem
  const char *p_idbus = udev_device_get_property_value(device.get(), "ID_BUS");
  if (p_idbus != NULL)
    id_bus_ = p_idbus;
  if (id_bus_ != "usb" && id_bus_ != "USB")
    throw std::logic_error("Wrong ID_BUS");
  // block name
  const char *p_block = udev_device_get_property_value(device.get(), "DEVNAME");
  if (p_block != NULL)
    block_name_ = p_block;
  if (block_name_.empty())
    throw std::logic_error("Empty block name");
  // fs label
  const char *p_label =
      udev_device_get_property_value(device.get(), "ID_FS_LABEL");
  if (p_label != NULL)
    fs_label_ = p_label;
  // fs uuid
  const char *p_uid =
      udev_device_get_property_value(device.get(), "ID_FS_UUID");
  if (p_uid != NULL)
    uid_ = p_uid;
  // fs type
  const char *p_fs = udev_device_get_property_value(device.get(), "ID_FS_TYPE");
  if (p_fs != NULL)
    filesystem_ = p_fs;
  const char *p_devtipe =
      udev_device_get_property_value(device.get(), "DEVTYPE");
  if (p_devtipe != NULL)
    dev_type_ = p_devtipe;
  const char *p_partition_number =
      udev_device_get_property_value(device.get(), "ID_PART_ENTRY_NUMBER");
  if (p_partition_number != NULL)
    partitions_number_ = std::stoi(p_partition_number);
  const char *p_vid =
      udev_device_get_property_value(device.get(), "ID_VENDOR_ID");
  if (p_vid != NULL)
    vid_ = p_vid;
  const char *p_pid =
      udev_device_get_property_value(device.get(), "ID_MODEL_ID");
  if (p_pid != NULL)
    pid_ = p_pid;
}

void UsbUdevDevice::SetAction(const char *p_action) {
  if (p_action != NULL) {
    std::string tmp = p_action;
    if (tmp == "add")
      action_ = Action::kAdd;
    else if (tmp == "remove")
      action_ = Action::kRemove;
    else
      action_ = Action::kUndefined;
    if (action_ == Action::kUndefined)
      throw std::logic_error("Undefined action for device");
  } else
    throw std::logic_error("Empty ACTION");
}

std::string UsbUdevDevice::toString() const noexcept {
  std::stringstream res;
  res << "Action: ";
  switch (action_) {
  case Action::kAdd:
    res << "add";
    break;
  case Action::kRemove:
    res << "remove";
    break;
  case Action::kUndefined:
    res << "undefined";
    break;
  }
  res << " block_name: " << block_name_ << " fs_label: " << fs_label_
      << " fs_uid: " << uid_ << " file_system: " << filesystem_
      << " device_type:" << dev_type_ << " partition number "
      << std::to_string(partitions_number_) << " vid " << vid_ << " pid "
      << pid_ << " subsystem " << subsystem_;
  return res.str();
}

} // namespace usbmount