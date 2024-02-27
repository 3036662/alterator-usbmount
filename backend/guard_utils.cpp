#include "guard_utils.hpp"
#include "json_rule.hpp"
#include "log.hpp"
#include "usb_device.hpp"
#include "utils.hpp"
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

std::optional<bool>
ExtractDaemonTargetState(boost::json::object *p_obj) noexcept {
  if (p_obj != nullptr && p_obj->contains("run_daemon") &&
      p_obj->at("run_daemon").is_string()) {
    return p_obj->at("run_daemon").as_string() == "true";
  }
  Log::Error() << "No target daemon state is found in JSON";
  return std::nullopt;
}

std::optional<Target> ExtractTargetPolicy(boost::json::object *p_obj) noexcept {
  if (p_obj != nullptr && p_obj->contains("policy_type") &&
      p_obj->at("policy_type").is_string()) {
    return p_obj->at("policy_type").as_string() == "radio_white_list"
               ? Target::block
               : Target::allow;
  }
  Log::Error() << "No target policy is found in JSON";
  return std::nullopt;
}

std::optional<std::string>
ExtractPresetMode(boost::json::object *p_obj) noexcept {
  if (p_obj != nullptr && p_obj->contains("preset_mode") &&
      p_obj->at("preset_mode").is_string()) {
    return p_obj->at("preset_mode").as_string().c_str();
  }
  Log::Error() << "No preset mode is found in JSON";
  return std::nullopt;
}

boost::json::object
ProcessJsonManualMode(const boost::json::object *ptr_jobj,
                      std::vector<uint> &rules_to_delete,
                      std::vector<GuardRule> &rules_to_add) noexcept {
  using ::utils::StrToUint;
  namespace json = boost::json;
  // if some new rules were added
  // put new rules to vector rules_to_add
  // put rules ids to obj_result["rules_OK"] and obj_result["rules_BAD"]
  json::object obj_result;
  obj_result["rules_OK"] = json::array();
  obj_result["rules_BAD"] = json::array();
  if (ptr_jobj != nullptr && ptr_jobj->contains("appended_rules")) {
    const json::array *ptr_json_array_rules =
        ptr_jobj->at("appended_rules").if_array();
    if (ptr_json_array_rules != nullptr && !ptr_json_array_rules->empty()) {
      obj_result = ProcessJsonAppended(ptr_json_array_rules, rules_to_add);
    }
  }
  // if we need to remove some rules
  // put rules numbers to "rules_to_delete" vector
  if (ptr_jobj->contains("deleted_rules") &&
      ptr_jobj->at("deleted_rules").is_array()) {
    for (const auto &element : ptr_jobj->at("deleted_rules").as_array()) {
      if (!element.is_string())
        continue;
      auto id_rule = StrToUint(element.as_string().c_str());
      if (id_rule.has_value()) {
        rules_to_delete.push_back(*id_rule);
      }
    }
  }
  // put list of validated string to response json
  if (obj_result.contains("rules_BAD")) {
    obj_result["STATUS"] =
        obj_result.at("rules_BAD").as_array().empty() ? "OK" : "BAD";
  } else {
    obj_result["STATUS"] = "BAD";
  }
  return obj_result;
}

boost::json::object
ProcessJsonAppended(const boost::json::array *ptr_json_array_rules,
                    std::vector<GuardRule> &rules_to_add) noexcept {
  using guard::utils::json::JsonRule;
  boost::json::array json_arr_OK;
  boost::json::array json_arr_BAD;
  for (const auto &rule : *ptr_json_array_rules) {
    const boost::json::object *ptr_json_rule = rule.if_object();
    if (ptr_json_rule != nullptr && ptr_json_rule->contains("tr_id")) {
      const boost::json::string *tr_id = ptr_json_rule->at("tr_id").if_string();
      // try to build a rule
      try {
        JsonRule json_rule(ptr_json_rule);
        GuardRule rule{json_rule.BuildString()};
        rules_to_add.push_back(std::move(rule));
        if (tr_id != nullptr && !tr_id->empty()) {
          json_arr_OK.emplace_back(*tr_id);
        }
      }
      // if failed
      catch (const std::logic_error &ex) {
        Log::Error() << "Can't build the rule";
        Log::Error() << ex.what();
        if (tr_id != nullptr && !tr_id->empty()) {
          json_arr_BAD.emplace_back(*tr_id);
        }
      }
    }
  }
  boost::json::object obj_result;
  obj_result["rules_OK"] = std::move(json_arr_OK);
  obj_result["rules_BAD"] = std::move(json_arr_BAD);
  return obj_result;
}

void AddAllowHid(std::vector<GuardRule> &rules_to_add) noexcept {
  try {
    rules_to_add.emplace_back("allow with-interface 03:*:*");
  } catch (const std::logic_error &ex) {
    Log::Error() << "Can't add a rules for HID devices";
    Log::Error() << ex.what();
  }
}

void AddBlockUsbStorages(std::vector<GuardRule> &rules_to_add) noexcept {
  try {
    rules_to_add.emplace_back("block with-interface 08:*:*");
    rules_to_add.emplace_back("block with-interface 06:*:*");
  } catch (const std::logic_error &ex) {
    Log::Error() << "Can't add a rules for USB storages";
    Log::Error() << ex.what();
  }
}

bool AddRejectAndroid(std::vector<GuardRule> &rules_to_add) noexcept {
  const std::string path_to_vidpid = "/etc/usbguard/android_vidpid.json";
  try {
    if (!std::filesystem::exists(path_to_vidpid)) {
      Log::Error() << "File " << path_to_vidpid << "doesn't exist.";
      return false;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't find file " << path_to_vidpid << "\n" << ex.what();
    return false;
  }
  std::stringstream buf;
  std::ifstream file_vid_pid(path_to_vidpid);
  if (!file_vid_pid.is_open()) {
    Log::Error() << "Can't open file" << path_to_vidpid;
    return false;
  }
  buf << file_vid_pid.rdbuf();
  file_vid_pid.close();

  boost::json::value json;
  boost::json::array *ptr_arr;
  try {
    json = boost::json::parse(buf.str());
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse a JSON file";
    return false;
  }
  ptr_arr = json.if_array();
  if (ptr_arr == nullptr || ptr_arr->empty()) {
    Log::Error() << "Empty json array";
    return false;
  }
  // for (auto it = ptr_arr->cbegin(); it != ptr_arr->cend(); ++it) {
  for (const auto &element : *ptr_arr) {
    if (element.is_object() && element.as_object().contains("vid") &&
        element.as_object().contains("pid")) {
      std::stringstream string_builder;
      string_builder << "block id "
                     << ::utils::UnQuote(
                            element.as_object().at("vid").as_string().c_str())
                     << ":"
                     << ::utils::UnQuote(
                            element.as_object().at("pid").as_string().c_str());
      try {
        rules_to_add.emplace_back(string_builder.str());
      } catch (const std::logic_error &ex) {
        Log::Warning() << "Error creating a rule from JSON";
        Log::Warning() << string_builder.str();
        return false;
      }
    }
  }
  return true;
}

} // namespace guard::utils