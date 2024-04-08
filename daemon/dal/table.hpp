#pragma once
#include "dto.hpp"
#include <boost/json/value.hpp>
#include <cstdint>
#include <exception>
#include <memory>

namespace usbmount::dal {

// CRUD table
class Table : public Dto {
public:
  Table(const std::string &data_file_path);
  json::value ToJson() const noexcept override;
  void Delete(uint64_t) noexcept;
  uint64_t size() const noexcept;
  void Clear() noexcept;

  virtual void Create(const Dto &) = 0;
  virtual const Dto &Read(uint64_t) const = 0;
  virtual void Update(uint64_t, const Dto &) = 0;
  virtual ~Table() = default;

protected:
  void CheckIndex(uint64_t index) const;

  std::string raw_json_;
  static constexpr const char *kWrongArg = "no data with such index";
  std::map<uint64_t, std::shared_ptr<Dto>> data_;

private:
  void ReadRaw();
  virtual void DataFromRawJson() = 0;
  const std::string file_path_; // path to data file
};

} // namespace usbmount::dal