#pragma once
#include "custom_mount.hpp"
#include "usb_udev_device.hpp"
#include <acl/libacl.h>
#include <chrono>
#include <libudev.h>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <sys/acl.h>
#include <sys/syslog.h>
#include <sys/types.h>

namespace usbmount::utils {

/**
 * @brief Logger initialization
 *
 * @param path string path to logfile
 * @return std::shared_ptr<spdlog::logger>
 */
std::shared_ptr<spdlog::logger> InitLogFile(const std::string &path) noexcept;

/**
 * @brief Mount or unmount device (depends on UsbUdevDevice action value )
 * @param ptr_device Device to process
 * @param logger
 */
void MountDevice(std::shared_ptr<UsbUdevDevice> ptr_device,
                 const std::shared_ptr<spdlog::logger> &logger) noexcept;

/**
 * @brief Check for expired mountpoints in local table
 */
bool ReviewMountPoints(const std::shared_ptr<spdlog::logger> &logger) noexcept;

/**
 * @brief Utility function for timing
 */
template <class result_t = std::chrono::milliseconds,
          class clock_t = std::chrono::high_resolution_clock,
          class duration_t = std::chrono::milliseconds>
auto since(std::chrono::time_point<clock_t, duration_t> const &start) {
  return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

namespace acl {
/**
 * @brief convert ACL to sting
 *
 * @return std::string
 */
std::string ToString(const acl_t &) noexcept;

/**
 * @brief Delete all ACL_USER and ACL_GROUP from ACL and MASK
 * @param acl
 */
void DeleteACLUserGroupMask(acl_t &acl);

} // namespace acl

} // namespace usbmount::utils