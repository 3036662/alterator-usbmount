#pragma once
#include <memory>
#include <mutex>

namespace usbmount::dao {

/**
 * @brief Local storage holds data about known devices and mountpoints.
 *
 */

class LocalStorage {
public:
  LocalStorage(const LocalStorage &) = delete;
  LocalStorage(LocalStorage &&) = delete;
  LocalStorage &operator=(const LocalStorage &) = delete;
  LocalStorage &operator=(LocalStorage &&) = delete;

  /**
   * @brief Get the Storage object
   * @return LocalStorage
   */
  static std::shared_ptr<LocalStorage> GetStorage();

private:
  LocalStorage();

  static std::shared_ptr<LocalStorage> p_instance_;
  static std::mutex mutex_;
};

} // namespace usbmount::dao