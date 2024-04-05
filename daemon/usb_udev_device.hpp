#pragma once
#include <libudev.h>
#include <memory>
#include <string>

namespace usbmount {

enum class Action { kRemove, kAdd, kUndefined };

class UsbUdevDevice {
public:
  UsbUdevDevice(
      std::unique_ptr<udev_device, decltype(&udev_device_unref)> &&device);
  std::string toString() const noexcept;

  // getters
  inline Action action() const noexcept { return action_; }
  inline const std::string &subsystem() const noexcept { return subsystem_; }
  inline const std::string &fs_label() const noexcept { return fs_label_; }
  inline const std::string &fs_uid() const noexcept { return uid_; }
  inline const std::string &filesystem() const noexcept { return filesystem_; }
  inline const std::string &dev_type() const noexcept { return dev_type_; }
  inline const std::string &block_name() const noexcept { return block_name_; }
  inline int partition_number() const noexcept { return partitions_number_; }

private:
  void SetAction(const char *p_action);

  Action action_ = Action::kUndefined;
  std::string subsystem_;
  std::string block_name_; /// /dev/sdX
  std::string id_bus_;     //  ID_BUS
  std::string fs_label_;   /// ID_FS_LABEL
  std::string uid_;        /// ID_FS_UUID
  std::string filesystem_; /// ID_FS_TYPE
  std::string dev_type_;   /// DEVTYPE e.g "partition"
  int partitions_number_ = 0;
  std::string vid_;
  std::string pid_;
};

} // namespace usbmount