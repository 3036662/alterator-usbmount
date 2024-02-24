#include "usb_device.hpp"
#include "log.hpp"
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>
namespace guard {

UsbDevice::UsbDevice(int num, const std::string &status_,
                     const std::string &name_, const std::string &vid_,
                     const std::string &pid_, const std::string &port_,
                     const std::string &connection_, const std::string &i_type_,
                     const std::string &sn_, const std::string &hash_)
    : number(num), status(status_), name(name_), vid(vid_), pid(pid_),
      port(port_), connection(connection_), i_type(i_type_), sn(sn_),
      hash(hash_) {}

vecPairs UsbDevice::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("label_prsnt_usb_number", std::to_string(number));
  res.emplace_back("label_prsnt_usb_port", port);
  res.emplace_back("label_prsnt_usb_class", i_type);
  res.emplace_back("label_prsnt_usb_vid", vid);
  res.emplace_back("label_prsnt_usb_pid", pid);
  res.emplace_back("label_prsnt_usb_status", status);
  res.emplace_back("label_prsnt_usb_name", name);
  // res.emplace_back("label_prsnt_usb_connection", connection);
  res.emplace_back("label_prsnt_usb_serial", sn);
  res.emplace_back("label_prsnt_usb_hash", hash);
  res.emplace_back("label_prsnt_usb_vendor", vendor_name);
  return res;
}

UsbType::UsbType(const std::string &str) {
  std::logic_error ex = std::logic_error("Can't parse CC::SS::PP " + str);
  std::vector<std::string> splitted;
  boost::split(splitted, str, [](const char c) { return c == ':'; });
  if (splitted.size() != 3) {
    throw ex;
  }
  for (std::string &el : splitted) {
    boost::trim(el);
    if (el.size() > 2) {
      throw ex;
    }
  }
  base = stoi(splitted[0], nullptr, 16);
  base_str = std::move(splitted[0]);
  sub = stoi(splitted[1], nullptr, 16);
  sub_str = std::move(splitted[1]);
  protocol = stoi(splitted[2], nullptr, 16);
  protocol_str = std::move(splitted[2]);
}

std::vector<std::string> FoldUsbInterfacesList(std::string i_type) {
  using guard::utils::Log;
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
    std::multiset<char> set;
    for (const UsbType &usb_type : vec_usb_types) {
      set.emplace(usb_type.base);
    }
    // if a key is not unique, create a mask.
    auto it_unique_end =
        std::unique(vec_usb_types.begin(), vec_usb_types.end(),
                    [](const UsbType &first, const UsbType &second) {
                      return first.base == second.base;
                    });
    for (auto it = vec_usb_types.begin(); it != it_unique_end; ++it) {
      size_t numb = set.count(it->base);
      std::string tmp = it->base_str;
      if (numb == 1) {
        tmp += ':';
        tmp += it->sub_str;
        tmp += ':';
        tmp += it->protocol_str;
      } else {
        tmp += ":*:*";
      }
      vec_i_types.emplace_back(std::move(tmp));
    }
  } else {
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

} // namespace guard