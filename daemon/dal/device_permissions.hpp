#pragma once
#include "table.hpp"
#include <cstdint>

namespace usbmount::dal {

class DevicePermissions : public Table {
public:
  DevicePermissions(const std::string &);

  // CRUD
  void Create(const Dto &) override;
  const PermissionEntry &Read(uint64_t) const override;
  void Update(uint64_t, const Dto &) override;

  std::optional<uint64_t> Find(const Device &dev) const noexcept;

private:
  void DataFromRawJson() override;
};

} // namespace usbmount::dal
