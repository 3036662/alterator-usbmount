#pragma once
#include "rapidcsv.h"
#include <string>

namespace guard::utils::csv {

class CsvRule {
public:
  explicit CsvRule(rapidcsv::Document &doc, size_t index);
  std::string BuildString() const noexcept;

private:
  std::string target_;
  std::string interface_;
  std::string vidpid_;
  std::string hash_;
};

} // namespace guard::utils::csv