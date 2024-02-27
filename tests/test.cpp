#include "config_status.hpp"
#include "guard.hpp"
#include "guard_rule.hpp"
#include "json_rule.hpp"
#include "log.hpp"
#include "systemd_dbus.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include "utils.hpp"

using guard::utils::Log;
using utils::WrapWithQuotes;


void Test::Run1() {
  Log::Test() << "Test udev rules files search";
  // guard::Guard guard_obj;
  std::string curr_path = std::filesystem::current_path().string();

  // create files
  const std::string file1 = curr_path + "/rule1.rules";
  const std::string file2 = curr_path + "/rule2.rules";
  const std::string file3 = curr_path + "/rule3.rules";
  const std::string file4 = curr_path + "/rule4.rules";
  const std::string file5 = curr_path + "/rule5.rules";
  const std::string file6 = curr_path + "/rule6.rules";

  // usb + authorized
  std::ofstream os(file1);
  if (os.is_open())
    os << "blalala usb blabla authorized" << std::endl;
  os.close();

  // not usb neither authorized
  os = std::ofstream(file2);
  if (os.is_open())
    os << "bla bla \n bla bla" << std::endl;
  os.close();

  // simple long file
  os = std::ofstream(file3);
  if (os.is_open()) {
    for (int i = 0; i < 2048; ++i)
      os << i;
  }
  os.close();

  // only usb
  os = std::ofstream(file4);
  if (os.is_open())
    os << "bla usb \n bla bla" << std::endl;
  os.close();

  // CASE different cases
  os = std::ofstream(file5);
  if (os.is_open())
    os << "bla uSB \n bla bla AutHorizeD"
       << "" << std::endl;
  os.close();

  // CASE different
  os = std::ofstream(file6);
  if (os.is_open())
    os << "bla uS \n bla bla AutHorizeD"
       << "" << std::endl;
  os.close();

  // check
  std::vector<std::string> vec_mock{
      curr_path,
      "sdgzg098gav\\c" // bad path
  };
  const std::unordered_map<std::string, std::string> map =
      guard::ConfigStatus::InspectUdevRules(&vec_mock);
  const std::unordered_map<std::string, std::string> expected_map{
      std::pair<std::string, std::string>{file1, "usb_rule"},
      std::pair<std::string, std::string>{file5, "usb_rule"},
      std::pair<std::string, std::string>{
          file6, "usb_rule"}}; // even if only authorized
  assert(map == expected_map);
  Log::Test() << "TEST1 ... OK";
}

void Test::Run2() {
  guard::ConfigStatus cs;

  if (std::system("systemctl stop usbguard")) {
    throw std::logic_error("can't stop usbguard");
  }
  if (std::system("systemctl disable usbguard")) {
    throw std::logic_error("can't stop usbguard");
  }

  cs.CheckDaemon();
  Log::Test() << " ACTIVE " << cs.guard_daemon_active << " ENABLED "
              << cs.guard_daemon_enabled << " DAEMON_OK " << cs.guard_daemon_OK
              << " UDEV_RULES_OK " << cs.udev_rules_OK;
  assert(cs.guard_daemon_active == false);
  assert(cs.guard_daemon_enabled == false);

  if (std::system("systemctl start usbguard")) {
    throw std::logic_error("can't start usbguard");
  }
  if (std::system("systemctl enable usbguard")) {
    throw std::logic_error("can't enable usbguard");
  }
  cs.CheckDaemon();
  Log::Test() << " ACTIVE " << cs.guard_daemon_active << " ENABLED "
              << cs.guard_daemon_enabled << " DAEMON_OK " << cs.guard_daemon_OK
              << " UDEV_RULES_OK " << cs.udev_rules_OK;
  assert(cs.guard_daemon_active == true);
  assert(cs.guard_daemon_enabled == true);
  Log::Test() << "TEST2 ... OK";
}

void Test::Run3() {
  guard::ConfigStatus cs;
  std::string res = cs.GetDaemonConfigPath();
  Log::Test() << "Config path is " << res;
  assert(std::filesystem::exists(res));
  Log::Test() << "TEST3 ... OK";
}

void Test::Run4() {
  guard::ConfigStatus cs;
  cs.ParseDaemonConfig();
  Log::Test() << "Found rules file " << cs.daemon_rules_file_path;
  Log::Test() << "Rules file exists = " << cs.rules_files_exists;
  assert(!cs.daemon_rules_file_path.empty() &&
         cs.daemon_rules_file_path == "/etc/usbguard/rules.conf" &&
         cs.rules_files_exists ==
             std::filesystem::exists(cs.daemon_rules_file_path));
  Log::Test() << "TEST4 ... OK";
}

void Test::Run5() {
  guard::ConfigStatus cs;
  cs.ParseDaemonConfig();
  Log::Test() << "Users found:";
  for (const std::string &user : cs.ipc_allowed_users) {
    Log::Test() << user;
  }

  const std::string path_to_users = "/etc/usbguard/IPCAccessControl.d/";
  std::set<std::string> expected_set = {"root"};
  assert(cs.ipc_allowed_users == expected_set);

  if (std::system("usbguard add-user test")) {
    throw std::logic_error("Can't add user to usbguard");
  }
  cs.ParseDaemonConfig();
  for (const std::string &user : cs.ipc_allowed_users) {
    Log::Test() << user;
  }
  expected_set.insert("test");
  assert(cs.ipc_allowed_users == expected_set);

  if (std::system("usbguard remove-user test")) {
    throw std::logic_error("Can't add user to usbguard");
  }
  cs.ParseDaemonConfig();
  for (const std::string &user : cs.ipc_allowed_users) {
    Log::Test() << user;
  }
  expected_set.erase("test");
  assert(cs.ipc_allowed_users == expected_set);

  Log::Test() << "TEST5 ... OK";
}

void Test::Run6() {
  guard::ConfigStatus cs;
  cs.ParseDaemonConfig();
  Log::Test() << "Groups found:";
  for (const std::string &group : cs.ipc_allowed_groups) {
    Log::Test() << group;
  }
  std::set<std::string> expected{"wheel"};
  assert(cs.ipc_allowed_groups == expected);
  Log::Test() << "TEST6 ... OK";
}

void Test::Run7() {
  guard::Guard guard;
  std::vector<std::string> res = guard::FoldUsbInterfacesList(
      "with-interface { 0e:01:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 "
      "0e:02:00 0e:02:00 0e:02:00 }");
  std::vector<std::string> exp = {"0e:*:*"};
  assert(res == exp);
  res = guard::FoldUsbInterfacesList("with-interface { 03:01:02 04:01:01 }");
  exp = {"03:01:02", "04:01:01"};
  assert(res == exp);
  res = guard::FoldUsbInterfacesList("with-interface 09:00:00");
  exp = {"09:00:00"};
  assert(res == exp);
  res = guard::FoldUsbInterfacesList("wsdsafsdga");
  exp = {"wsdsafsdga"};
  assert(res == exp);
  res = guard::FoldUsbInterfacesList("");
  exp = {""};
  assert(res == exp);

  res = guard::FoldUsbInterfacesList("with-interface { 03=01=:02 04:/1:01 }");
  exp = {};
  assert(res == exp);
  Log::Test() << "TEST7 .... OK";
};

void Test::Run8() {
  guard::Guard guard;

  std::unordered_set<std::string> vendors{"03e9", "03ea", "04c5", "06cd"};

  std::unordered_map<std::string, std::string> expected{
      {"03e9", "Thesys Microelectronics"},
      {"03ea", "Data Broadcasting Corp."},
      {"04c5", "Fujitsu, Ltd"},
      {"06cd", "Keyspan"}};
  assert(guard::MapVendorCodesToNames(vendors) == expected);

  vendors.insert("06cf");
  expected.emplace("06cf", "SpheronVR AG");
  assert(guard::MapVendorCodesToNames(vendors) == expected);
  vendors.insert("blablalbla");
  assert(guard::MapVendorCodesToNames(vendors) == expected);
  assert(guard::MapVendorCodesToNames(vendors).size() < vendors.size());
  Log::Test() << "TEST8  ... OK";
}

void Test::Run9() {
  guard::GuardRule parser("allow");
  {
    std::vector<std::string> expected{"a", "b"};
    assert(parser.SplitRawRule("a b") == expected);
  }

  {
    std::vector<std::string> expected{"a", "b"};
    assert(parser.SplitRawRule("a b ") == expected);
  }

  {
    std::vector<std::string> expected{"a", "b"};
    assert(parser.SplitRawRule("a    b ") == expected);
  }

  {
    std::vector<std::string> expected{"a", "b"};
    assert(parser.SplitRawRule("      a    b ") == expected);
  }

  {
    std::vector<std::string> expected{"\"a b\"", "c"};
    assert(parser.SplitRawRule("      \"a b\"    c ") == expected);
  }

  {
    std::vector<std::string> expected{"\"a b\"", "\"c d e\"", "fff", "ggg"};
    assert(parser.SplitRawRule("      \"a b\"    \"c d e\" fff     ggg ") ==
           expected);
  }

  {
    std::vector<std::string> expected{};
    assert(parser.SplitRawRule("   ") == expected);
  }

  {
    std::vector<std::string> expected{"\"\"", "\" \""};
    assert(parser.SplitRawRule(" \"\"    \" \" ") == expected);
  }
  Log::Test() << "TEST9 ...OK";
}

void Test::Run10() {
  std::string str = "allow";
  {
    guard::GuardRule parser(str);
    assert((parser.target == guard::Target::allow) && !parser.vid &&
           !parser.pid && !parser.hash && !parser.device_name &&
           !parser.serial && !parser.port && !parser.with_interface &&
           !parser.cond);
  }
  str = "block id 8564:1000";
  {
    guard::GuardRule parser(str);
    assert((parser.target == guard::Target::block) && *parser.vid == "8564" &&
           *parser.pid == "1000" && !parser.hash && !parser.device_name &&
           !parser.serial && !parser.port && !parser.with_interface &&
           !parser.cond);
  }
  str = "block id 8564:*";
  {
    guard::GuardRule parser(str);
    assert((parser.target == guard::Target::block) && *parser.vid == "8564" &&
           *parser.pid == "*" && !parser.hash && !parser.device_name &&
           !parser.serial && !parser.port && !parser.with_interface &&
           !parser.cond);
  }

  str = "block id 8564:* hash 4Q3Ski/Lqi8RbTFr10zFlIpagY9AKVMszyzBQJVKE+c=";
  {
    guard::GuardRule parser(str);
    assert((parser.target == guard::Target::block) && *parser.vid == "8564" &&
           *parser.pid == "*" &&
           *parser.hash == "4Q3Ski/Lqi8RbTFr10zFlIpagY9AKVMszyzBQJVKE+c=" &&
           !parser.device_name && !parser.serial && !parser.port &&
           !parser.with_interface && !parser.cond);
  }

  str = "block hash 4Q3Ski/Lqi8RbTFr10zFlIpagY9AKVMszyzBQJVKE+c=";
  {
    guard::GuardRule parser(str);
    assert((parser.target == guard::Target::block) && !parser.vid &&
           !parser.pid &&
           *parser.hash == "4Q3Ski/Lqi8RbTFr10zFlIpagY9AKVMszyzBQJVKE+c=" &&
           !parser.device_name && !parser.serial && !parser.port &&
           !parser.with_interface && !parser.cond);
  }

  // name
  str = "block hash 4Q3Ski/Lqi8RbTFr10zFlIpagY9AKVMszyzBQJVKE+c= name \"USB "
        "name   _      Lastname\" ";
  {
    guard::GuardRule parser(str);
    assert((parser.target == guard::Target::block) && !parser.vid &&
           !parser.pid &&
           *parser.hash == "4Q3Ski/Lqi8RbTFr10zFlIpagY9AKVMszyzBQJVKE+c=" &&
           *parser.device_name == "\"USB name   _      Lastname\"" &&
           !parser.serial && !parser.port && !parser.with_interface &&
           !parser.cond);
  }

  str = "block hash 4Q3Ski/Lqi8RbTFr10zFlIpagY9AKVMszyzBQJVKE+c= name \"USB "
        "name   _      Lastname\"  serial \"4098lkjmd\"  ";
  {
    guard::GuardRule parser(str);
    assert((parser.target == guard::Target::block) && !parser.vid &&
           !parser.pid &&
           *parser.hash == "4Q3Ski/Lqi8RbTFr10zFlIpagY9AKVMszyzBQJVKE+c=" &&
           *parser.device_name == "\"USB name   _      Lastname\"" &&
           *parser.serial == "\"4098lkjmd\"" && !parser.port &&
           !parser.with_interface && !parser.cond);
  }

  // exceptions

  str = "block hash 4Q3Sk";
  {
    try {
      guard::GuardRule parser(str);
    } catch (const std::logic_error &ex) {
      Log::Test() << "Catched expected exception";
      assert(std::string(ex.what()) == "Cant parse rule string");
    }
  }

  str = " ";
  {
    try {
      guard::GuardRule parser(str);
    } catch (const std::logic_error &ex) {
      Log::Test() << "Catched expected exception";
      assert(std::string(ex.what()) == "Cant parse rule string");
    }
  }

  str = "";
  {
    try {
      guard::GuardRule parser(str);
    } catch (const std::logic_error &ex) {
      Log::Test() << "Catched expected exception";
      assert(std::string(ex.what()) == "Cant parse rule string");
    }
  }

  // one port
  str = "allow via-port \"1-2\"";
  {
    guard::GuardRule parser(str);
    std::vector<std::string> ports_expected{"\"1-2\""};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_port{
        guard::RuleOperator::no_operator, ports_expected};

    assert(parser.target == guard::Target::allow && !parser.vid &&
           !parser.pid && !parser.hash && !parser.device_name &&
           !parser.serial && *parser.port == expected_port &&
           !parser.with_interface && !parser.cond);
  }

  // two ports
  str = "allow via-port all-of {" + WrapWithQuotes("1-2") + " " +
        WrapWithQuotes("2-2") + "}";
  {
    guard::GuardRule parser(str);
    std::vector<std::string> ports_expected{"\"1-2\"", "\"2-2\""};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_port{
        guard::RuleOperator::all_of, ports_expected};

    assert(parser.target == guard::Target::allow && !parser.vid &&
           !parser.pid && !parser.hash && !parser.device_name &&
           !parser.serial && *parser.port == expected_port &&
           !parser.with_interface && !parser.cond);
  }

  str = "allow via-port all-of {" + WrapWithQuotes("1-2") + " " +
        WrapWithQuotes("2-2") + "";
  {
    try {
      guard::GuardRule parser(str);
    } catch (const std::logic_error &ex) {
      Log::Test() << "Catched expected exception";
      Log::Test() << ex.what();
    }
  }

  // two ports one interface
  str = "allow via-port all-of {" + WrapWithQuotes("1-2") + " " +
        WrapWithQuotes("2-2") + "} with-interface 08:06:50";
  {
    guard::GuardRule parser(str);
    std::vector<std::string> ports_expected{"\"1-2\"", "\"2-2\""};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_port{
        guard::RuleOperator::all_of, ports_expected};

    std::vector<std::string> interface_expected = {"08:06:50"};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_interface{
        guard::RuleOperator::no_operator, interface_expected};

    assert(parser.target == guard::Target::allow && !parser.vid &&
           !parser.pid && !parser.hash && !parser.device_name &&
           !parser.serial && *parser.port == expected_port &&
           *parser.with_interface == expected_interface && !parser.cond);
  }

  // two ports three interfaces
  str = "allow via-port all-of {" + WrapWithQuotes("1-2") + " " +
        WrapWithQuotes("2-2") + "} with-interface none-of{08:06:50 07:*:*}";
  {
    guard::GuardRule parser(str);
    std::vector<std::string> ports_expected{"\"1-2\"", "\"2-2\""};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_port{
        guard::RuleOperator::all_of, ports_expected};

    std::vector<std::string> interface_expected = {"08:06:50", "07:*:*"};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_interface{
        guard::RuleOperator::none_of, interface_expected};

    assert(parser.target == guard::Target::allow && !parser.vid &&
           !parser.pid && !parser.hash && !parser.device_name &&
           !parser.serial && *parser.port == expected_port &&
           *parser.with_interface == expected_interface && !parser.cond);
  }

  // conditioins
  Log::Test() << "START CONDITIONS TEST";

  // clang-format off
  using namespace guard;

Log::Test() << "Localtime time with parameter";
  str = "allow if localtime(00:00-12:00)";
  {
    GuardRule parser(str);
    RuleWithOptionalParam rule_with_param(RuleConditions::localtime,"00:00-12:00");
    RuleWithBool rule_with_bool {true,rule_with_param};
    std::vector<RuleWithBool> vec;
    vec.push_back(rule_with_bool);
    std::pair<RuleOperator,std::vector<RuleWithBool>> pair {RuleOperator::no_operator,vec};


    assert(parser.target ==guard::Target::allow &&
          !parser.pid &&
          !parser.hash &&
          !parser.device_name &&
          !parser.serial &&
          !parser.port &&
          !parser.with_interface &&
          parser.cond == pair
    );

  }

  //******************************************************************
  Log::Test() << "Localtime time with unclosed braces ";
  str = "allow if localtime(00:00-12:00";
  {
     bool cought{false};
    try{
      GuardRule parser(str);
    }
    catch (const std::logic_error& ex){
      Log::Test() << "Cought an expected exception.";
      cought =true;
    }
    assert(cought);
  }
  //******************************************************************
  Log::Test() << "Localtime time with unclosed braces ";
  str = "allow if localtime)00:00-12:00)";
  
  {
    bool cought{false};
    try{
      GuardRule parser(str);
    }
    catch (const std::logic_error& ex){
      Log::Test() << "Cought an expected exception.";
      cought =true;
    }
    assert(cought);
  }

  //******************************************************************

Log::Test() << "Rule-applied time without parameter";
  str = "allow if !rule-applied";
  {
    GuardRule parser(str);
    RuleWithOptionalParam rule_with_param(RuleConditions::rule_applied,std::nullopt);
    RuleWithBool rule_with_bool {false,rule_with_param};
    std::vector<RuleWithBool> vec;
    vec.push_back(rule_with_bool);
    std::pair<RuleOperator,std::vector<RuleWithBool>> pair {RuleOperator::no_operator,vec};

    Log::Test() << parser.ConditionsToString();

    assert(parser.target ==guard::Target::allow &&
          !parser.pid &&
          !parser.hash &&
          !parser.device_name &&
          !parser.serial &&
          !parser.port &&
          !parser.with_interface &&
          parser.cond == pair
    );
  }


Log::Test() << "rule-applied time with parameter";
  str = "allow if !rule-applied(HH__SMM_MM)";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="!rule-applied(HH__SMM_MM)";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
  }

Log::Test() << "rule-evaluated time with no parameter";
  str = "allow if !rule-evaluated";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="!rule-evaluated";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
  }

  Log::Test() << "rule-evaluated with  parameters";
  str = "allow if rule-evaluated(HH:MM:SS)";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="rule-evaluated(HH:MM:SS)";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
  }

  Log::Test() << "One-of  with sequense of coditions";
  str = "allow if one-of{!rule-evaluated(HH:MM:SS) true}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{!rule-evaluated(HH:MM:SS) true}";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
  }

  Log::Test()<< "id, none-of  with sequense of coditions";
  str = "allow id 8564:1000 if one-of{!rule-evaluated(HH:MM:SS) true}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{!rule-evaluated(HH:MM:SS) true}";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }

// real rule
 str="allow id 1d6b:0002 serial \"0000:00:0d.0\" name \"xHCI Host Controller\" hash \"d3YN7OD60Ggqc9hClW0/al6tlFEshidDnQKzZRRk410=\" parent-hash \"Y1kBdG1uWQr5CjULQs7uh2F6pHgFb6VDHcWLk83v+tE=\" with-interface 09:00:00 with-connect-type \"\""; 
  {
    GuardRule parser(str);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="1d6b");
    assert(*parser.pid=="0002");
    assert(!parser.cond.has_value());
    assert(*parser.device_name=="\"xHCI Host Controller\"");
    assert(*parser.hash=="\"d3YN7OD60Ggqc9hClW0/al6tlFEshidDnQKzZRRk410=\"");
    assert(!parser.port.has_value());
    assert(*parser.serial=="\"0000:00:0d.0\"");
    assert(parser.with_interface->first == guard::RuleOperator::no_operator);
    assert(parser.with_interface->second.at(0) == "09:00:00");
  }

  Log::Test() << "id, none-of  with sequense of coditions +localtime";
  str = "allow id 8564:1000 if one-of{localtime(HH:MM:SS) true}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{localtime(HH:MM:SS) true}";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }


  Log::Test() << "id, one-of  with sequense of coditions +allow-matches";
  str = "allow id 8564:1000 if one-of{allowed-matches(query) localtime(HH:MM:SS)}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{allowed-matches(query) localtime(HH:MM:SS)}";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }

   Log::Test() << "[TEST] id, one-of  with sequense of coditions +allow-matches ++ rule-applied(past_duration)";
  str = "allow id 8564:1000 if one-of{allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration)}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration)}";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }

    Log::Test() << "id, one-of  with sequense of coditions +allow-matches ++ rule-applied(past_duration)  rule-applied  + random(p_true) +random";
  str = "allow id 8564:1000 if one-of{allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration) rule-applied random(p_true)}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration) rule-applied random(p_true)}";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }

  Log::Test() << "id, one-of  with sequense of coditions  true +allow-matches ++ rule-applied(past_duration)  rule-applied  + random(p_true) +random";
  str = "allow id 8564:1000 if one-of{true allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration) rule-applied random(p_true)}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{true allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration) rule-applied random(p_true)}";
    Log::Test() <<cond_result <<"==" << expected;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }

  Log::Test() << "allowed-matches without params fails";
  str = "allow id 8564:1000 if allowed-matches";
  {
    try{
      GuardRule parser(str);
      std::string cond_result=parser.ConditionsToString();
    }
    catch (const std::logic_error& ex){
      assert(std::string(ex.what()) == "No parameters found for condition allowed-matches");
    }
  }

  Log::Test() << "allowed-matches without params fails";
  str = "allow if true)";
  {
    try{
      GuardRule parser(str);
      assert(parser.BuildString()=="allow if true");
      
    }
    catch (const std::logic_error& ex){
      assert(std::string(ex.what()) == "Some text was found after a condition");
    }
  }
  
  {
    Log::Test() << "empty array fails";
    str = "allow id 1000:2000 hash \"sdaasdklkjd\" name \"device_name\" via-port all-of{}";
    try{
      GuardRule parser(str);
      
    }
    catch (const std::logic_error& ex){
      assert(std::string(ex.what()) == "Empty array {} is not supported");
    }
  }

  str="allow id 30c9:0030 serial \"0001\" name \"Integrated Camera\" hash \"94ed2Mm6HGRsDZTjqV8TdnQWRDdUvlDdTmMm+henvVk=\" parent-hash \"jEP/6WzviqdJ5VSeTUY8PatCNBKeaREvo2OqdplND/o=\" with-interface { 0e:01:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 } with-connect-type \"hardwired\"";
  {
    GuardRule parser(str);
  } 


  str="allow id 1d6b:0002 serial \"0000:03:00.0\" name \"xHCI Host Controller\" hash \"Lw/Cdah32MiEGYi1D+rX5Vcs8544WKd6bqSOuVKqKn4=\" parent-hash \"w3c++Hva/cvMNTcx3y72UlxkR0WUPCWne2mlaosYanw=\" via-port \"usb1\" with-interface 09:00:00 with-connect-type \"\"";
  {
    GuardRule parser(str);
    assert (parser.BuildString() ==str);
  }  

  Log::Test() << "TEST10 .... OK";
}

void Test::Run11(){
  
  // parse real file

   guard::Guard guard;
   guard::ConfigStatus cs (guard.GetConfigStatus());
   std::vector<guard::GuardRule> result = cs.ParseGuardRulesFile().first;

  Log::Test() <<"parse config rules. Build each rule from object,compare with the source. ";

  std::ifstream file(cs.daemon_rules_file_path);
  std::string line;
  int line_counter=0;
  while(std::getline(file,line)){
    Log::Test() <<"=================================";
    Log::Test() << "RULE NUMBER " << line_counter;
    Log::Test() << line;
    Log::Test() << result[line_counter].BuildString(true,true);
    assert(line == result[line_counter].BuildString(true,true));
    line.clear();
    ++line_counter;
  }
  file.close();

  Log::Test() << "TEST11 ... OK";

}



void Test::Run13(){
  dbus_bindings::Systemd sd;

  Log::Test() << "TEST13 Test usbguard start stop and restart";
  auto init_state=sd.IsUnitActive("usbguard.service");
  assert (init_state.has_value());
  Log::Test() << "Start service ...";
  // start if stopped
  if (init_state && !init_state.value()){
    if (std::system("systemctl start usbguard")) {
      throw std::logic_error("can't start usbguard");
    }
    {
    auto val=sd.IsUnitActive("usbguard.service");
    assert (val.has_value() && val.value());
    }
  }
  Log::Test() <<"OK";
 std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // stop
  Log::Test() <<"Stop service ...";
  {
    auto val=sd.StopUnit("usbguard.service");
    assert (val.has_value() && val.value());
  }
  Log::Test() <<"OK";

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // start
  Log::Test() << "Start service ...";
  {
    auto val=sd.StartUnit("usbguard.service");
    assert (val.has_value() && val.value());
  }
  Log::Test() <<"OK";

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // restart
   Log::Test() << "Restart service ...";
  {
    auto val=sd.IsUnitActive("usbguard.service");
    assert (val.has_value() && val.value());
  }
  Log::Test() <<"OK";
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //  return to init state
  if (init_state && !init_state.value()){
    sd.StopUnit("usbguard.service");
  }

  Log::Test() << "TEST13 ... OK";
}

// JSON RULE  OBJECTS PARSING

void Test::Run14(){
  Log::Test() <<" Test GuardRule building from json string";

  {
    Log::Test() << "Normal json with vid pid ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},{\"pid\":\"a5a5\"}]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "a5a5");
    assert(rule.level == guard::StrictnessLevel::vid_pid);

  }

   {
    Log::Test() << "Empty pid ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"\"},{\"pid\":\"a5a5\"}]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();  
    try{                 
    guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    }
    catch(const std::logic_error& ex){
      assert(std::string (ex.what()) == "Empty value for field vid" );
    }
  }

  {
    Log::Test() << "Empty pid ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"aaa\"},{\"pid\":\"\"}]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();  
    try{                 
          guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    }
    catch(const std::logic_error& ex){
      assert(std::string (ex.what()) == "Empty value for field pid" );
    }
  }

   {
    Log::Test() << "Empty fields arr ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();  
    try{                 
          guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    }
    catch(const std::logic_error& ex){
      assert(std::string (ex.what()) == "Can't find any fields for a rule" );
    }
  }

  {
    Log::Test() << "Empty target ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"fields_arr\":"
                        "[{\"vid\":\"aaa\"},{\"pid\":\"\"}]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();  
    try{                 
        guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    }
    catch(const std::logic_error& ex){
      assert(std::string (ex.what()) == "Rule target is mandatory" );
    }
  }


  {
    Log::Test() << "Normal with hash ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"a5a5\"},"
                         "{\"hash\":\"salkdjlskjf\"}"
                        "]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
       guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "a5a5");
    assert(rule.hash == "salkdjlskjf");
    assert(rule.level == guard::StrictnessLevel::hash);

  }


  {
    Log::Test() << "With hash and interface ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"a5a5\"},"
                         "{\"hash\":\"salkdjlskjf\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" }"
                        "]}]}";         
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
        guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "a5a5");
    assert(rule.hash == "salkdjlskjf");
    assert(rule.level == guard::StrictnessLevel::hash);

    std::pair<guard::RuleOperator,std::vector<std::string>> pair=std::make_pair(guard::RuleOperator::equals,std::vector<std::string>());
    pair.second.push_back("04:00:*");
    pair.second.push_back("01:11:00");

    assert(rule.with_interface.has_value());
    assert(rule.with_interface.value() == pair);
  }


  {
    Log::Test() << "Bad interface ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"a5a5\"},"
                         "{\"hash\":\"salkdjlskjf\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00 sal;dfk;}\" }"
                        "]}]}";                   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();  
    try{                 
          guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    }
    catch(const std::logic_error& ex){
         assert(std::string(ex.what()) == "Cant parse rule string");
    }
   
  }

  {
    Log::Test() << "With vidpid and interface ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"a5a5\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" }"
                        "]}]}";       
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
       guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "a5a5");

    std::pair<guard::RuleOperator,std::vector<std::string>> pair=std::make_pair(guard::RuleOperator::equals,std::vector<std::string>());
    pair.second.push_back("04:00:*");
    pair.second.push_back("01:11:00");

    assert(rule.with_interface.has_value());
    assert(rule.with_interface.value() == pair);
    assert(rule.level == guard::StrictnessLevel::vid_pid);
  }

  
  {
    Log::Test() << "With only an interface ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"block\",\""
                        "fields_arr\":["
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" }"
                        "]}]}";          
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
        guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    
    assert(rule.target == guard::Target::block);


    assert(rule.with_interface.has_value());
    assert(rule.level == guard::StrictnessLevel::interface);
  }

   {
    Log::Test()<< "With only an interface ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"block\",\""
                        "fields_arr\":["
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" }"
                        "]}]}";           
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
       guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    
    assert(rule.target == guard::Target::block);
    assert(rule.with_interface.has_value());
    assert(rule.level == guard::StrictnessLevel::interface);
  }

  {
    Log::Test() << "With vidpid, interface, ports ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"a5a5\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" },"
                         "{\"via-port\":\"none-of {usb1 usb2 usb3}\"}"
                        "]}]}";   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
        guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "a5a5");

    std::pair<guard::RuleOperator,std::vector<std::string>> pair=std::make_pair(guard::RuleOperator::equals,std::vector<std::string>());
    pair.second.push_back("04:00:*");
    pair.second.push_back("01:11:00");

    std::pair<guard::RuleOperator,std::vector<std::string>> ports=std::make_pair(guard::RuleOperator::none_of,std::vector<std::string>());
    ports.second.push_back("usb1");
    ports.second.push_back("usb2");
    ports.second.push_back("usb3");

    assert(rule.with_interface.has_value());
    assert(rule.with_interface.value() == pair);
    assert(rule.port.has_value());
    assert(rule.port == ports);
    assert(rule.level == guard::StrictnessLevel::vid_pid);
  }

  {
    Log::Test() << "With vidpid, interface, ports,conn type ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"a5a5\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" },"
                         "{\"via-port\":\"none-of {usb1 usb2 usb3}\"},"
                         "{\"with-connect-type\":\"hotplug\"}"
                        "]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
       guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "a5a5");

    std::pair<guard::RuleOperator,std::vector<std::string>> pair=std::make_pair(guard::RuleOperator::equals,std::vector<std::string>());
    pair.second.push_back("04:00:*");
    pair.second.push_back("01:11:00");

    std::pair<guard::RuleOperator,std::vector<std::string>> ports=std::make_pair(guard::RuleOperator::none_of,std::vector<std::string>());
    ports.second.push_back("usb1");
    ports.second.push_back("usb2");
    ports.second.push_back("usb3");

    assert(rule.with_interface.has_value());
    assert(rule.with_interface.value() == pair);
    assert(rule.port.has_value());
    assert(rule.port == ports);
    assert(rule.level == guard::StrictnessLevel::vid_pid);
    assert(rule.conn_type == "hotplug");
  }

  {
    Log::Test()<< "With vidpid, interface, ports,conn type, conditions ";
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"a5a5\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" },"
                         "{\"via-port\":\"none-of {usb1 usb2 usb3}\"},"
                         "{\"with-connect-type\":\"hotplug\"},"
                         "{\"cond\":\"if one-of {!localtime(00:00:00) true}\"}"
                        "]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
        guard::utils::json::JsonRule json_rule(ptr_obj);
    guard::GuardRule rule(json_rule.BuildString());
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "a5a5");

    std::pair<guard::RuleOperator,std::vector<std::string>> pair=std::make_pair(guard::RuleOperator::equals,std::vector<std::string>());
    pair.second.push_back("04:00:*");
    pair.second.push_back("01:11:00");

    std::pair<guard::RuleOperator,std::vector<std::string>> ports=std::make_pair(guard::RuleOperator::none_of,std::vector<std::string>());
    ports.second.push_back("usb1");
    ports.second.push_back("usb2");
    ports.second.push_back("usb3");

    assert(rule.with_interface.has_value());
    assert(rule.with_interface.value() == pair);
    assert(rule.port.has_value());
    assert(rule.port == ports);
    assert(rule.level == guard::StrictnessLevel::vid_pid);
    assert(rule.conn_type == "hotplug");

    std::pair<guard::RuleOperator, std::vector<guard::RuleWithBool>> conds=std::make_pair(guard::RuleOperator::one_of,std::vector<guard::RuleWithBool>());
    guard::RuleWithOptionalParam rule_with_param1 (guard::RuleConditions::localtime,"00:00:00");
    guard::RuleWithOptionalParam rule_with_param2 (guard::RuleConditions::always_true,{});

    conds.second.emplace_back(false,rule_with_param1); 
    conds.second.emplace_back(true,rule_with_param2);
    assert(rule.cond.has_value());
    assert(rule.cond->first == guard::RuleOperator::one_of);
    assert(rule.cond == conds );
  }
  
  Log::Test() << "[TEST] TEST 14 ... OK";
}

void Test::Run15(){

  {
    guard::Guard guard;
    std::string json ="{\"policy_type\":\"radio_white_list\", \"preset_mode\":\"manual_mode\",\"deleted_rules\":null,\"appended_rules\":[],\"run_daemon\":\"true\"}"; 
    auto res=guard.ParseJsonRulesChanges(json);
    assert(res.has_value());
    assert(*res =="{\"rules_OK\":[],\"rules_BAD\":[],\"STATUS\":\"OK\"}" );
  }


  {
    //stop daemon
    guard::Guard guard;
    std::string json ="{\"policy_type\":\"radio_white_list\",\"preset_mode\":\"manual_mode\",\"deleted_rules\":null,\"appended_rules\":[],\"run_daemon\":\"false\"}"; 
    auto res=guard.ParseJsonRulesChanges(json);
    assert(res.has_value());
    assert(*res =="{\"rules_OK\":[],\"rules_BAD\":[],\"STATUS\":\"OK\"}" );

    guard::ConfigStatus cs;
    assert(!cs.guard_daemon_active);
    assert(!cs.guard_daemon_enabled);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  {
    // run daemon
    guard::Guard guard;
    std::string json ="{\"policy_type\":\"radio_white_list\",\"preset_mode\":\"manual_mode\",\"deleted_rules\":null,\"appended_rules\":[],\"run_daemon\":\"true\"}"; 
    auto res=guard.ParseJsonRulesChanges(json);
    assert(res.has_value());
    assert(*res =="{\"rules_OK\":[],\"rules_BAD\":[],\"STATUS\":\"OK\"}" );

    guard::ConfigStatus cs;
    assert(cs.guard_daemon_active);
    assert(cs.guard_daemon_enabled);
  }

  Log::Test() << "[TEST] TEST 15 ... OK";
}

void Test::Run16(){
  Log::Test() << "Test changing implicit policy";
  guard::ConfigStatus cs;
  auto init_policy=cs.implicit_policy_target;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  cs.ChangeImplicitPolicy(false);
   Log::Test() << "cs.policy "<< cs.implicit_policy_target ;
  assert(cs.implicit_policy_target =="allow");
  
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  cs.ChangeImplicitPolicy(true);
  Log::Test() << "cs.policy "<< cs.implicit_policy_target;
  assert(cs.implicit_policy_target =="block");
  
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  cs.ChangeImplicitPolicy(init_policy=="block");
  assert( cs.implicit_policy_target==init_policy);
  Log::Test() << "TEST 16 ... OK";
}

void Test::Run17(){
  Log::Test() << " Test allow connected ";
  Log::Warning() <<"Test with sleep 10sec";
std::this_thread::sleep_for(std::chrono::milliseconds(10000));
Log::Info() <<"Sleep ...";
{
  guard::Guard guard;
  std::string json ="{\"policy_type\":\"radio_white_list\",\"preset_mode\":\"put_connected_to_white_list\",\"deleted_rules\":[],\"appended_rules\":[],\"run_daemon\":\"true\"}";
  auto result=guard.ParseJsonRulesChanges(json);
  assert (result);
  Log::Test() <<*result;
  assert (result=="{\"STATUS\":\"OK\"}");
  assert (guard.GetConfigStatus().guard_daemon_active);
  assert (guard.GetConfigStatus().guard_daemon_enabled);
}

Log::Info() <<"Sleep ...";
std::this_thread::sleep_for(std::chrono::milliseconds(10000));
{
  guard::Guard guard;
  std::string json ="{\"policy_type\":\"radio_white_list\",\"preset_mode\":\"put_connected_to_white_list\",\"deleted_rules\":null,\"appended_rules\":[],\"run_daemon\":\"false\"}";
  auto result=guard.ParseJsonRulesChanges(json);
  assert (result);
  assert (result=="{\"STATUS\":\"OK\"}");
  assert (!guard.GetConfigStatus().guard_daemon_active);
  assert (!guard.GetConfigStatus().guard_daemon_enabled);
}
Log::Info() <<"Sleep ...";
std::this_thread::sleep_for(std::chrono::milliseconds(10000));
{
  guard::Guard guard;
  std::string json ="{\"policy_type\":\"radio_black_list\",\"preset_mode\":\"manual_mode\",\"deleted_rules\":null,\"appended_rules\":[],\"run_daemon\":\"false\"}";
  auto result=guard.ParseJsonRulesChanges(json);
  assert (result);
  Log::Test() <<*result;
  assert (result=="{\"rules_OK\":[],\"rules_BAD\":[],\"STATUS\":\"OK\"}");
  assert (!guard.GetConfigStatus().guard_daemon_active);
  assert (!guard.GetConfigStatus().guard_daemon_enabled);
}
  Log::Test() << "TEST 17 ... OK";
}