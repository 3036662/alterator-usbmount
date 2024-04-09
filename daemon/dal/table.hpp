#pragma once
#include "dto.hpp"
#include <boost/json/value.hpp>
#include <cstdint>
#include <exception>
#include <memory>
#include <shared_mutex>

namespace usbmount::dal {

// CRUD table
class Table : public Dto {
public:
  Table(const std::string &data_file_path);
  json::value ToJson() const noexcept override;
  void Delete(uint64_t);
  uint64_t size() const noexcept;
  void Clear();

  virtual void Create(const Dto &) = 0;
  virtual const Dto &Read(uint64_t) const = 0;
  virtual void Update(uint64_t, const Dto &) = 0;
  virtual ~Table() = default;

protected:
  void CheckIndex(uint64_t index) const;
  void WriteRaw();

  std::string raw_json_;
  static constexpr const char *kWrongArg = "no data with such index";
  std::map<uint64_t, std::shared_ptr<Dto>> data_;
  mutable std::shared_mutex data_mutex_;

private:
  void ReadRaw();
  virtual void DataFromRawJson() = 0;
  const std::string file_path_; // path to data file
  std::shared_mutex file_mutex_;
};

} // namespace usbmount::dal