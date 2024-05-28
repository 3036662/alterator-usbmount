/* File: table.hpp

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
#include <boost/json/value.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace usbmount::dal {

// CRUD table
class Table : public Dto {
public:
  explicit Table(const std::string &data_file_path);
  Table(const Table &) = delete;
  Table(Table &&) = delete;
  Table &operator=(const Table &) = delete;
  Table &operator=(Table &&) = delete;
  ~Table() override = default;
  json::value ToJson() const noexcept override;

  void Clear();
  uint64_t size() const noexcept;

  virtual void Create(const Dto &) = 0;
  virtual const Dto &Read(uint64_t) const = 0;
  virtual void Update(uint64_t, const Dto &) = 0;
  void Delete(uint64_t);

  /**
   * @brief Transactions can be used for modifing method - CREATE,UPDATE,DELETE
   */
  void StartTransaction() noexcept;
  bool ProcessTransaction() noexcept;

protected:
  void CheckIndex(uint64_t index) const;
  void WriteRaw();
  // NOLINTBEGIN
  std::mutex transaction_mutex_;
  bool transaction_started_ = false;
  std::string raw_json_;
  static constexpr const char *kWrongArg = "no data with such index";
  std::map<uint64_t, std::shared_ptr<Dto>> data_;
  mutable std::shared_mutex data_mutex_;
  std::shared_mutex file_mutex_;
  // NOLINTEND

private:
  void ReadRaw();
  virtual void DataFromRawJson() = 0;
  void DeepDataClone();

  const std::string file_path_; // path to data file

  std::map<uint64_t, std::shared_ptr<Dto>> data_clone_;
  std::unique_lock<std::shared_mutex> transaction_data_lock_;
  std::unique_lock<std::shared_mutex> transaction_file_lock_;
};

} // namespace usbmount::dal