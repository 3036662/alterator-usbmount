#pragma once
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>

struct DevParams {
  std::string dev_path;
  std::string action;
};

enum class Action { kRemove, kAdd, kUndefined };

class UsbUdevDevice {
public:
  UsbUdevDevice() = delete;

  /**
   * @brief Construct a new Usb Udev Device object
   *
   * @param params DevParams struct {dev_path,action}
   * @param logger
   * @throws std::runtime_error
   */
  UsbUdevDevice(const DevParams &params,
                const std::shared_ptr<spdlog::logger> &logger);

  std::string toString() const noexcept;

private:
  /**
   * @brief Get the Udev Device Info for block device
   */
  void getUdevDeviceInfo();

  const std::shared_ptr<spdlog::logger> logger_;

  Action action_;
  std::string block_name_;
  std::string fs_label_;   /// ID_FS_LABEL
  std::string uid_;        /// ID_PART_ENTRY_UUID
  std::string filesystem_; /// ID_FS_TYPE
  std::string dev_type_;   /// DEVTYPE e.g "partition"
  int partitions_number_;
  // std::string vid_;
  // std::string pid_;
  // std::string subsystem_;
};