#include "guard_audit.hpp"
#include "guard.hpp"
#include "log.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace guard {

using guard::utils::Log;

GuardAudit::GuardAudit(AuditType type, const std::string &path)
    : audit_type_(type), audit_file_path_(path) {
  if (type == AuditType::kUndefined)
    throw std::logic_error("Undefined audit type");
  if (type == AuditType::kFileAudit && path.empty())
    throw std::logic_error("The path to audit file is empty");
  if (type == AuditType::kFileAudit && !std::filesystem::exists(path)) {
    Log::Warning() << "USBGuard audit file doesn't exist";
  }
  if (type == AuditType::kLinuxAudit)
    throw std::logic_error("LinuxAudit is used");
}

std::vector<std::string> GuardAudit::GetAll() const noexcept {
  return GetByFilter({});
}

std::vector<std::string> GuardAudit::GetByFilter(
    const std::vector<std::string> &filters) const noexcept {
  if (audit_type_ == AuditType::kFileAudit) {
    try {
      return GetFromFile(filters);
    } catch (const std::exception &ex) {
      Log::Error() << ex.what();
      return {};
    }
  }
  return {};
}

std::vector<std::string>
GuardAudit::GetFromFile(const std::vector<std::string> &filters) const {
  std::vector<std::string> res;
  if (audit_file_path_.empty())
    throw std::logic_error("Empty path to audit file");
  if (!std::filesystem::exists(audit_file_path_))
    throw std::logic_error("Audit file doesn't exist");
  std::ifstream file(audit_file_path_);
  if (!file.is_open()) {
    Log::Error() << "Can't open " << audit_file_path_;
    throw std::runtime_error("Can't open audit file");
  }
  std::string line;
  while (std::getline(file, line)) {
    if (filters.empty() || std::any_of(filters.cbegin(), filters.cend(),
                                       [&line](const std::string &filter) {
                                         return boost::contains(line, filter);
                                       }))
      res.emplace_back(line);
    line.clear();
  }
  file.close();
  return res;
}

} // namespace guard