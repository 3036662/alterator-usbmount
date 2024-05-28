/* File: device_permissions.hpp

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

#pragma once
#include "dto.hpp"
#include "table.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace usbmount::dal {

class DevicePermissions : public Table {
public:
  /**
   * @brief Construct a new DevicePermissions object
   * @param path Path to a file to store data
   * @throws runtime_error
   */
  explicit DevicePermissions(const std::string &);

  // CRUD

  /**
   * @brief  Create a new entry for a permisson in the local storage
   * @throws bad_cast , runtime_error
   */
  void Create(const Dto &dto) override;

  /**
   * @brief Get entry by index
   * @return const MountEntry&
   * @throws std::invalid_argument (wrong index), out_of_range by std::map
   */
  const PermissionEntry &Read(uint64_t index) const override;

  /**
   * @brief Update entry by index
   * @throws bad_cast, invalid_argument (index)
   */
  void Update(uint64_t index, const Dto &dto) override;

  /**
   * @brief Find by Device object
   * @param dev dal::Device object
   * @return std::optional<uint64_t> index or empty if nothing was found
   */
  std::optional<uint64_t> Find(const Device &dev) const noexcept;

  std::map<uint64_t, std::shared_ptr<const PermissionEntry>>
  getAll() const noexcept;

private:
  /**
   * @brief Read raw_json_ and fill the fields with data
   * @throws runtime_error, system_error (json  parser)
   */
  void DataFromRawJson() override;

  // no cloning
  std::shared_ptr<Dto> Clone() const noexcept override { return nullptr; };
};

} // namespace usbmount::dal
