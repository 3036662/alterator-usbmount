#include "guard_utils.hpp"
#include "log.hpp"
#include "usb_device.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <set>

namespace guard::utils {

std::vector<std::string> FoldUsbInterfacesList(std::string i_type) {
  boost::erase_first(i_type, "with-interface");
  boost::trim(i_type);
  std::vector<std::string> vec_i_types;
  // if a multiple types in one string
  if (i_type.find('{') != std::string::npos &&
      i_type.find('}') != std::string::npos) {
    std::vector<std::string> splitted;
    boost::erase_all(i_type, "{");
    boost::erase_all(i_type, "}");
    boost::trim(i_type);
    boost::split(splitted, i_type,
                 [](const char symbol) { return symbol == ' '; });
    // create sequence of usb types
    std::vector<UsbType> vec_usb_types;
    for (const std::string &usb_type : splitted) {
      try {
        vec_usb_types.emplace_back(usb_type);
      } catch (const std::exception &e) {
        Log::Error() << "Can't parse a usb type" << usb_type;
        Log::Error() << e.what();
      }
    }
    // fold if possible
    // put to multiset bases
    std::multiset<unsigned char> set;
    for (const UsbType &usb_type : vec_usb_types) {
      set.emplace(usb_type.base());
    }
    // if a key is not unique, create a mask.
    auto it_unique_end =
        std::unique(vec_usb_types.begin(), vec_usb_types.end(),
                    [](const UsbType &first, const UsbType &second) {
                      return first.base() == second.base();
                    });
    for (auto it = vec_usb_types.begin(); it != it_unique_end; ++it) {
      size_t numb = set.count(it->base());
      std::string tmp = it->base_str();
      if (numb == 1) {
        tmp += ':';
        tmp += it->sub_str();
        tmp += ':';
        tmp += it->protocol_str();
      } else {
        tmp += ":*:*";
      }
      vec_i_types.emplace_back(std::move(tmp));
    }
  } else {
    try {
      UsbType tmp(i_type);
    } catch (const std::exception &ex) {
      Log::Error() << "Can't parse a usb type " << i_type;
      Log::Error() << ex.what();
    }
    // push even if parsing failed
    vec_i_types.push_back(std::move(i_type));
  }
  return vec_i_types;
}

std::unordered_map<std::string, std::string>
MapVendorCodesToNames(const std::unordered_set<std::string> &vendors) noexcept {
  using guard::utils::Log;
  std::unordered_map<std::string, std::string> res;
  const std::string path_to_usb_ids = "/usr/share/misc/usb.ids";
  try {
    if (std::filesystem::exists(path_to_usb_ids)) {
      std::ifstream filestream(path_to_usb_ids);
      if (filestream.is_open()) {
        std::string line;
        while (getline(filestream, line)) {
          // not interested in strings starting with tab
          if (!line.empty() && line[0] == '\t') {
            line.clear();
            continue;
          }
          auto range = boost::find_first(line, "  ");
          if (static_cast<bool>(range)) {
            std::string vendor_id(line.begin(), range.begin());
            if (vendors.count(vendor_id) != 0) {
              std::string vendor_name(range.end(), line.end());
              res.emplace(std::move(vendor_id), std::move(vendor_name));
            }
          }
          line.clear();
        }
        filestream.close();
      } else {
        Log::Warning() << "Can't open file " << path_to_usb_ids;
      }
    } else {
      Log::Error() << "The file " << path_to_usb_ids << "doesn't exist";
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't map vendor IDs to vendor names.";
    Log::Error() << ex.what();
  }
  return res;
}

} // namespace guard::utils