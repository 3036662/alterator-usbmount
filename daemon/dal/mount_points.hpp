/* File: mount_points.hpp

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
#include <optional>
#include <unordered_set>
#include <vector>

namespace usbmount::dal {

class Mountpoints : public Table {
public:
  /**
   * @brief Construct a new Mountpoints object
   * @param path Path to a file to store data
   * @throws runtime_error
   */
  explicit Mountpoints(const std::string &path);

  // CRUD

  /**
   * @brief  Create a new entry for a mountpoint in the local storage
   * @throws bad_cast , runtime_error
   */
  void Create(const Dto &) override;

  /**
   * @brief Get entry by index
   * @return const MountEntry&
   * @throws std::invalid_argument (wrong index), out_of_range by std::map
   */
  const MountEntry &Read(uint64_t) const override;

  /**
   * @brief Update entry by index
   * @throws bad_cast, invalid_argument (index)
   */
  void Update(uint64_t, const Dto &) override;

  /**
   * @brief Find entry
   *
   * @param entry
   * @return std::optional<uint64_t> index or empry if nothing was found
   */
  std::optional<uint64_t> Find(const MountEntry &entry) const noexcept;

  /**
   * @brief Find entry by block device name
   *
   * @param block_dev e.g. "/dev/sda1"
   * @return std::optional<uint64_t> index or empry if nothing was found
   */
  std::optional<uint64_t> Find(const std::string &block_dev) const noexcept;

  /**
   * @brief Remove expired values from the mount_points table
   * @param valid_set The set of valid mountpoints
   * @details Valid set is supposed to be a /etc/mtab set.
   */
  void RemoveExpired(const std::unordered_set<std::string> &valid_set) noexcept;

  /**
   * @brief Get the All MountEnries
   * @return std::vector<MountEntry>
   */
  std::vector<MountEntry> GetAll() const noexcept;

private:
  /**
   * @brief Read raw_json_ and fill the fields with data
   * @throws runtime_error, system_error (json  parser)
   */
  void DataFromRawJson() override;

  // no cloning
  inline std::shared_ptr<Dto> Clone() const noexcept override {
    return nullptr;
  };
};

} // namespace usbmount::dal