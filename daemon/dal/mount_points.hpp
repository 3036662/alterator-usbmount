#pragma once
#include "table.hpp"

namespace usbmount::dal {

class Mountpoints : public Table {
public:
  Mountpoints(const std::string &path);

  // CRUD
  void Create(const Dto &) override;
  const MountEntry &Read(uint64_t) const override;
  void Update(uint64_t, const Dto &) override;

private:
  void DataFromRawJson() override;
};

} // namespace usbmount::dal