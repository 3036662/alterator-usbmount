/* File: log_reader.cpp

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

#include "log_reader.hpp"
#include "log.hpp"
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace common_utils {

namespace fs = std::filesystem;
using common_utils::Log;

LogReader::LogReader(std::string fpath) : log_file_path_(std::move(fpath)) {
  if (log_file_path_.empty()) {
    throw std::invalid_argument("Empty filepath");
  }
}

vecstring LogReader::GetFromFile(const vecstring &filters) const {
  vecstring res;
  if (log_file_path_.empty()) {
    throw std::logic_error("An empty path to file");
  }
  if (!fs::exists(log_file_path_)) {
    throw std::logic_error("File doesn't exist");
  }
  std::ifstream file(log_file_path_);
  if (!file.is_open()) {
    throw std::runtime_error("Can't open file");
  }
  std::string line;
  size_t line_number = 0;
  while (std::getline(file, line)) {
    ++line_number;
    if (filters.empty() || std::any_of(filters.cbegin(), filters.cend(),
                                       [&line](const std::string &filter) {
                                         return boost::contains(line, filter);
                                       })) {
      std::stringstream log_entry;
      log_entry << "[" << std::to_string(line_number) << "] " << line;
      res.emplace_back(log_entry.str());
    }
    line.clear();
  }
  file.close();
  return res;
}

vecstring LogReader::GetByFilter(const vecstring &filters) const noexcept {
  try {
    return GetFromFile(filters);
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
    return {};
  }
}

vecstring LogReader::GetAll() const noexcept { return GetByFilter({}); }

PageData LogReader::GetByPage(const vecstring &filters,
                              unsigned int page_number,
                              unsigned int pages_size) const noexcept {
  PageData data;
  data.curr_page = page_number;
  data.pages_number = 0;
  try {
    std::vector<std::string> all_lines = GetFromFile(filters);
    if (pages_size > 0) {
      const uint num_lines = static_cast<uint>(all_lines.size());
      data.pages_number = num_lines / pages_size;
      if (data.pages_number * pages_size < num_lines) {
        ++data.pages_number;
      }
    }
    const int lines_size = static_cast<int>(all_lines.size());
    int index_last = lines_size - static_cast<int>(page_number * pages_size);
    int index_first =
        lines_size - static_cast<int>((page_number + 1) * pages_size);
    if (index_first < 0) {
      index_first = 0;
    }
    if (index_last < 0) {
      index_last = 0;
    }
    if (index_last > lines_size) {
      index_last = lines_size;
    }
    if (index_first >= index_last) {
      return data;
    }
    for (auto i = static_cast<size_t>(index_first);
         i < static_cast<size_t>(index_last); ++i) {
      data.data.emplace_back(std::move(all_lines.at(i)));
    }
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
  }
  return data;
}

} // namespace common_utils