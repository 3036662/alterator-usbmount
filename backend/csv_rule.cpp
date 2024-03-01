#include "csv_rule.hpp"
#include "log.hpp"
#include "utils.hpp"
#include <cstddef>
#include <stdexcept>

namespace guard::utils::csv {

CsvRule::CsvRule(rapidcsv::Document &doc, size_t index) {
  std::vector<std::string> row = doc.GetRow<std::string>(index);
  size_t n_cols = row.size();
  if (row.empty())
    throw std::logic_error("Empty csv row");
  target_ = std::move(row[0]);
  if (target_.empty())
    throw std::logic_error("Empty rule target");
  if (n_cols >= 2)
    interface_ = row[1];
  if (n_cols >= 3)
    vidpid_ = row[2];
  if (n_cols >= 4)
    hash_ =row[3];
  if (n_cols >= 5)
    raw_ = row[4];
  guard::utils::Log::Debug()
      << "interface_= " << interface_ << "vidpid_=" << vidpid_
      << "hash_=" << hash_ << "raw=" << raw_;
}

std::string CsvRule::BuildString() const noexcept {
  std::ostringstream string_builder;
  if (!raw_.empty()) {
    string_builder << raw_;
  } else {
    string_builder << target_ << " ";
    if (!vidpid_.empty())
      string_builder << "id " << vidpid_;
    if (!device_name_.empty())
      string_builder << "name " << ::utils::QuoteIfNotQuoted(device_name_)
                     << " ";
    if (!hash_.empty())
      string_builder << "hash " << ::utils::QuoteIfNotQuoted(hash_) << " ";
    if (!interface_.empty())
      string_builder << "with-interface " << interface_ << " ";
  }
  return string_builder.str();
}

} // namespace guard::utils::csv