#include "guard.hpp"
#include "guard_rule.hpp"
#include "systemd_dbus.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

void Test::Run1() {
  std::cout << "Test udev rules files search" << std::endl;
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
      guard::InspectUdevRules(&vec_mock);
  const std::unordered_map<std::string, std::string> expected_map{
      std::pair<std::string, std::string>{file1, "usb_rule"},
      std::pair<std::string, std::string>{file5, "usb_rule"},
      std::pair<std::string, std::string>{
          file6, "usb_rule"}}; // even if only authorized
  assert(map == expected_map);
  std::cout << "TEST1 ... OK" << std::endl;
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
  std::cout << " ACTIVE " << cs.guard_daemon_active << " ENABLED "
            << cs.guard_daemon_enabled << " DAEMON_OK " << cs.guard_daemon_OK
            << " UDEV_RULES_OK " << cs.udev_rules_OK << std::endl;
  assert(cs.guard_daemon_active == false);
  assert(cs.guard_daemon_enabled == false);

  if (std::system("systemctl start usbguard")) {
    throw std::logic_error("can't start usbguard");
  }
  if (std::system("systemctl enable usbguard")) {
    throw std::logic_error("can't enable usbguard");
  }
  cs.CheckDaemon();
  std::cout << " ACTIVE " << cs.guard_daemon_active << " ENABLED "
            << cs.guard_daemon_enabled << " DAEMON_OK " << cs.guard_daemon_OK
            << " UDEV_RULES_OK " << cs.udev_rules_OK << std::endl;
  assert(cs.guard_daemon_active == true);
  assert(cs.guard_daemon_enabled == true);
  std::cout << "TEST2 ... OK" << std::endl;
}

void Test::Run3() {
  guard::ConfigStatus cs;
  std::string res = cs.GetDaemonConfigPath();
  std::cerr << "Config path is " << res << std::endl;
  assert(std::filesystem::exists(res));
  std::cout << "TEST3 ... OK" << std::endl;
}

void Test::Run4() {
  guard::ConfigStatus cs;
  cs.ParseDaemonConfig();
  std::cerr << "Found rules file " << cs.daemon_rules_file_path << std::endl;
  std::cerr << "Rules file exists = " << cs.rules_files_exists << std::endl;
  assert(!cs.daemon_rules_file_path.empty() &&
         cs.daemon_rules_file_path == "/etc/usbguard/rules.conf" &&
         cs.rules_files_exists ==
             std::filesystem::exists(cs.daemon_rules_file_path));
  std::cout << "TEST4 ... OK" << std::endl;
}

void Test::Run5() {
  guard::ConfigStatus cs;
  cs.ParseDaemonConfig();
  std::cerr << "Users found:" << std::endl;
  for (const std::string &user : cs.ipc_allowed_users) {
    std::cerr << user << std::endl;
  }

  const std::string path_to_users = "/etc/usbguard/IPCAccessControl.d/";
  std::set<std::string> expected_set = {"root"};
  assert(cs.ipc_allowed_users == expected_set);

  if (std::system("usbguard add-user test")) {
    throw std::logic_error("Can't add user to usbguard");
  }
  cs.ParseDaemonConfig();
  for (const std::string &user : cs.ipc_allowed_users) {
    std::cerr << user << std::endl;
  }
  expected_set.insert("test");
  assert(cs.ipc_allowed_users == expected_set);

  if (std::system("usbguard remove-user test")) {
    throw std::logic_error("Can't add user to usbguard");
  }
  cs.ParseDaemonConfig();
  for (const std::string &user : cs.ipc_allowed_users) {
    std::cerr << user << std::endl;
  }
  expected_set.erase("test");
  assert(cs.ipc_allowed_users == expected_set);

  std::cerr << "TEST5 ... OK" << std::endl;
}

void Test::Run6() {
  guard::ConfigStatus cs;
  cs.ParseDaemonConfig();
  std::cerr << "Groups found:" << std::endl;
  for (const std::string &group : cs.ipc_allowed_groups) {
    std::cerr << group << std::endl;
  }
  std::set<std::string> expected{"wheel"};
  assert(cs.ipc_allowed_groups == expected);
  std::cerr << "TEST6 ... OK" << std::endl;
}

void Test::Run7() {
  guard::Guard guard;
  std::vector<std::string> res = guard.FoldUsbInterfacesList(
      "with-interface { 0e:01:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 "
      "0e:02:00 0e:02:00 0e:02:00 }");
  std::vector<std::string> exp = {"0e:*:*"};
  assert(res == exp);
  res = guard.FoldUsbInterfacesList("with-interface { 03:01:02 04:01:01 }");
  exp = {"03:01:02", "04:01:01"};
  assert(res == exp);
  res = guard.FoldUsbInterfacesList("with-interface 09:00:00");
  exp = {"09:00:00"};
  assert(res == exp);
  res = guard.FoldUsbInterfacesList("wsdsafsdga");
  exp = {"wsdsafsdga"};
  assert(res == exp);
  res = guard.FoldUsbInterfacesList("");
  exp = {""};
  assert(res == exp);

  res = guard.FoldUsbInterfacesList("with-interface { 03=01=:02 04:/1:01 }");
  exp = {};
  assert(res == exp);
  std::cerr << "TEST7 .... OK" << std::endl;
};

void Test::Run8() {
  guard::Guard guard;

  std::unordered_set<std::string> vendors{"03e9", "03ea", "04c5", "06cd"};

  std::unordered_map<std::string, std::string> expected{
      {"03e9", "Thesys Microelectronics"},
      {"03ea", "Data Broadcasting Corp."},
      {"04c5", "Fujitsu, Ltd"},
      {"06cd", "Keyspan"}};
  assert(guard.MapVendorCodesToNames(vendors) == expected);

  vendors.insert("06cf");
  expected.emplace("06cf", "SpheronVR AG");
  assert(guard.MapVendorCodesToNames(vendors) == expected);
  vendors.insert("blablalbla");
  assert(guard.MapVendorCodesToNames(vendors) == expected);
  assert(guard.MapVendorCodesToNames(vendors).size() < vendors.size());
  std::cerr << "TEST8  ... OK" << std::endl;
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
  std::cerr << "TEST9 ...OK" << std::endl;
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
      std::cerr << "Catched expected exception" << std::endl;
      assert(std::string(ex.what()) == "Cant parse rule string");
    }
  }

  str = " ";
  {
    try {
      guard::GuardRule parser(str);
    } catch (const std::logic_error &ex) {
      std::cerr << "Catched expected exception" << std::endl;
      assert(std::string(ex.what()) == "Cant parse rule string");
    }
  }

  str = "";
  {
    try {
      guard::GuardRule parser(str);
    } catch (const std::logic_error &ex) {
      std::cerr << "Catched expected exception" << std::endl;
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
    // std::cerr << "START" << std::endl;
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
    // std::cerr << "START" << std::endl;
    try {
      guard::GuardRule parser(str);
    } catch (const std::logic_error &ex) {
      std::cerr << "Catched expected exception" << std::endl;
      std::cerr << ex.what() << std::endl;
    }
  }

  // two ports one interface
  str = "allow via-port all-of {" + WrapWithQuotes("1-2") + " " +
        WrapWithQuotes("2-2") + "} with-interface 08:06:50";
  {
    // std::cerr << "START" << std::endl;
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
  std::cerr << "[TEST] START CONDITIONS TEST" << std::endl;

  // clang-format off
  using namespace guard;

std::cerr << "[TEST] Localtime time with parameter" <<std::endl;
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
  std::cerr << "[TEST] Localtime time with unclosed braces " <<std::endl;
  str = "allow if localtime(00:00-12:00";
  {
     bool cought{false};
    try{
      GuardRule parser(str);
    }
    catch (const std::logic_error& ex){
      std::cerr << "[TEST] Cought an expected exception." <<std::endl;
      cought =true;
    }
    assert(cought);
  }
  //******************************************************************
  std::cerr << "[TEST] Localtime time with unclosed braces " <<std::endl;
  str = "allow if localtime)00:00-12:00)";
  
  {
    bool cought{false};
    try{
      GuardRule parser(str);
    }
    catch (const std::logic_error& ex){
      std::cerr << "[TEST] Cought an expected exception." <<std::endl;
      cought =true;
    }
    assert(cought);
  }

  //******************************************************************

std::cerr << "[TEST] rule-applied time without parameter" <<std::endl;
  str = "allow if !rule-applied";
  {
    GuardRule parser(str);
    RuleWithOptionalParam rule_with_param(RuleConditions::rule_applied,std::nullopt);
    RuleWithBool rule_with_bool {false,rule_with_param};
    std::vector<RuleWithBool> vec;
    vec.push_back(rule_with_bool);
    std::pair<RuleOperator,std::vector<RuleWithBool>> pair {RuleOperator::no_operator,vec};

    std::cerr << parser.ConditionsToString()<<std::endl;

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


std::cerr << "[TEST] rule-applied time with parameter" <<std::endl;
  str = "allow if !rule-applied(HH__SMM_MM)";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="!rule-applied(HH__SMM_MM)";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
    assert(cond_result == expected);
  }

std::cerr << "[TEST] rule-evaluated time with no parameter" <<std::endl;
  str = "allow if !rule-evaluated";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="!rule-evaluated";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
    assert(cond_result == expected);
  }

  std::cerr << "[TEST] rule-evaluated with  parameters" <<std::endl;
  str = "allow if rule-evaluated(HH:MM:SS)";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="rule-evaluated(HH:MM:SS)";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
    assert(cond_result == expected);
  }

  std::cerr << "[TEST] one-of  with sequense of coditions" <<std::endl;
  str = "allow if one-of{!rule-evaluated(HH:MM:SS) true}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{!rule-evaluated(HH:MM:SS) true}";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
    assert(cond_result == expected);
  }

  std::cerr << "[TEST] id, none-of  with sequense of coditions" <<std::endl;
  str = "allow id 8564:1000 if one-of{!rule-evaluated(HH:MM:SS) true}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{!rule-evaluated(HH:MM:SS) true}";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
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



  std::cerr << "[TEST] id, none-of  with sequense of coditions +localtime" <<std::endl;
  str = "allow id 8564:1000 if one-of{localtime(HH:MM:SS) true}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{localtime(HH:MM:SS) true}";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }


  std::cerr << "[TEST] id, one-of  with sequense of coditions +allow-matches" <<std::endl;
  str = "allow id 8564:1000 if one-of{allowed-matches(query) localtime(HH:MM:SS)}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{allowed-matches(query) localtime(HH:MM:SS)}";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }

   std::cerr << "[TEST] id, one-of  with sequense of coditions +allow-matches ++ rule-applied(past_duration)" <<std::endl;
  str = "allow id 8564:1000 if one-of{allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration)}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration)}";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }

    std::cerr << "[TEST] id, one-of  with sequense of coditions +allow-matches ++ rule-applied(past_duration)  rule-applied  + random(p_true) +random" <<std::endl;
  str = "allow id 8564:1000 if one-of{allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration) rule-applied random(p_true)}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration) rule-applied random(p_true)}";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }

  std::cerr << "[TEST] id, one-of  with sequense of coditions  true +allow-matches ++ rule-applied(past_duration)  rule-applied  + random(p_true) +random" <<std::endl;
  str = "allow id 8564:1000 if one-of{true allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration) rule-applied random(p_true)}";
  {
    GuardRule parser(str);
    std::string cond_result=parser.ConditionsToString();
    std::string expected="one-of{true allowed-matches(query) localtime(HH:MM:SS) rule-applied(past_duration) rule-applied random(p_true)}";
    std::cerr <<cond_result <<"==" << expected<<std::endl;
    assert(cond_result == expected);
    assert(parser.target == Target::allow);
    assert(*parser.vid=="8564");
    assert(*parser.pid=="1000");
  }

  std::cerr << "[TEST] allowed-matches without params fails" <<std::endl;
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

  std::cerr << "[TEST] allowed-matches without params fails" <<std::endl;
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
    std::cerr << "[TEST] empty array fails" <<std::endl;
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

  std::cerr << "TEST10 .... OK" << std::endl;
}

void Test::Run11(){
  
  // parse real file

   guard::Guard guard;
   guard::ConfigStatus cs (guard.GetConfigStatus());
   std::vector<guard::GuardRule> result = cs.ParseGuardRulesFile().first;

  std::cerr <<"[TEST] parse config rules. Build each rule from object,compare with the source. "<<std::endl;

  std::ifstream file(cs.daemon_rules_file_path);
  std::string line;
  int line_counter=0;
  while(std::getline(file,line)){
    std::cerr <<"\n=================================" << std::endl;
    std::cerr << "\n[TEST] RULE NUMBER " << line_counter<< "\n" << line << "\n" << result[line_counter].BuildString(true,true) <<std::endl;
    assert(line == result[line_counter].BuildString(true,true));
    line.clear();
    ++line_counter;
  }
  file.close();

  std::cerr << "[TEST] TEST11 ... OK"<<std::endl;

}

void Test::Run12(){
  std::cerr << "[TEST Test 12. Parsing json uint array" <<std::endl;
  {
    std::vector<uint> excpeted{12};
    std::string json ="[\"12\"]";
    assert (ParseJsonIntArray(json)== excpeted);
  }

  {
    std::vector<uint> excpeted{12,0,0,122};
    std::string json ="[" +WrapWithQuotes("12")+","+
                          WrapWithQuotes("0")+","+
                          WrapWithQuotes("0")+","+
                          WrapWithQuotes("122")+"]";
    assert (ParseJsonIntArray(json)== excpeted);
  }

  {
    std::vector<uint> excpeted{};
    std::string json ="";
    assert (ParseJsonIntArray(json)== excpeted);
  }

  {
    std::vector<uint> excpeted{};
    std::string json ="[]";
    assert (ParseJsonIntArray(json)== excpeted);
  }
  {
    std::vector<uint> excpeted{};
    std::string json ="[\"ы\"]";
    assert (ParseJsonIntArray(json)== excpeted);
  }
  std::cerr << "[TEST] TEST12 ... OK" <<std::endl;
}



void Test::Run13(){
  dbus_bindings::Systemd sd;

  std::cerr << "\n[TEST] TEST13 Test usbguard start stop and restart"<<std::endl;
  auto init_state=sd.IsUnitActive("usbguard.service");
  assert (init_state.has_value());
  std::cerr << "[TEST] Start service ...";
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
  std::cerr <<"OK"<< std::endl;
 std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // stop
  std::cerr <<"[TEST] Stop service ...";
  {
    auto val=sd.StopUnit("usbguard.service");
    assert (val.has_value() && val.value());
  }
  std::cerr <<"OK"<<std::endl;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // start
  std::cerr << "[TEST] Start service ...";
  {
    auto val=sd.StartUnit("usbguard.service");
    assert (val.has_value() && val.value());
  }
  std::cerr <<"OK"<<std::endl;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // restart
   std::cerr << "[TEST] Restart service ...";
  {
    auto val=sd.IsUnitActive("usbguard.service");
    assert (val.has_value() && val.value());
  }
  std::cerr <<"OK"<<std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //  return to init state
  if (init_state && !init_state.value()){
    sd.StopUnit("usbguard.service");
  }

  std::cerr << "[TEST] TEST13 ... OK" <<std::endl;
}

// JSON RULE  OBJECTS PARSING

void Test::Run14(){
  std::cerr<< "[TEST] Test GuardRule building from json string"<<std::endl;

  {
    std::cerr <<"[TEST] "<< "Normal json with vid pid "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},{\"pid\":\"s5a5\"}]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::GuardRule rule(ptr_obj);
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "s5a5");
    assert(rule.level == guard::StrictnessLevel::vid_pid);

  }

   {
    std::cerr <<"[TEST] "<< "Empty pid "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"\"},{\"pid\":\"s5a5\"}]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();  
    try{                 
      guard::GuardRule rule(ptr_obj);
    }
    catch(const std::logic_error& ex){
      assert(std::string (ex.what()) == "Empty value for field vid" );
    }
  }

  {
    std::cerr <<"[TEST] "<< "Empty pid "<<std::endl;
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
      guard::GuardRule rule(ptr_obj);
    }
    catch(const std::logic_error& ex){
      assert(std::string (ex.what()) == "Empty value for field pid" );
    }
  }

   {
    std::cerr <<"[TEST] "<< "Empty fields arr "<<std::endl;
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
      guard::GuardRule rule(ptr_obj);
    }
    catch(const std::logic_error& ex){
      //std::cerr <<ex.what();
      assert(std::string (ex.what()) == "Can't find any fields for a rule" );
    }
  }

  {
    std::cerr <<"[TEST] "<< "Empty target "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"fields_arr\":"
                        "[{\"vid\":\"aaa\"},{\"pid\":\"\"}]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();  
    try{                 
      guard::GuardRule rule(ptr_obj);
    }
    catch(const std::logic_error& ex){
      assert(std::string (ex.what()) == "Rule target is mandatory" );
    }
  }


  {
    std::cerr <<"[TEST] "<< "Normal with hash "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"s5a5\"},"
                         "{\"hash\":\"salkdjlskjf\"}"
                        "]}]}";
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::GuardRule rule(ptr_obj);
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "s5a5");
    assert(rule.hash == "salkdjlskjf");
    assert(rule.level == guard::StrictnessLevel::hash);

  }


  {
    std::cerr <<"[TEST] "<< "With hash and interface "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"s5a5\"},"
                         "{\"hash\":\"salkdjlskjf\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" }"
                        "]}]}";
    // std::cerr <<json<<std::endl;                   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::GuardRule rule(ptr_obj);
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "s5a5");
    assert(rule.hash == "salkdjlskjf");
    assert(rule.level == guard::StrictnessLevel::hash);

    std::pair<guard::RuleOperator,std::vector<std::string>> pair=std::make_pair(guard::RuleOperator::equals,std::vector<std::string>());
    pair.second.push_back("04:00:*");
    pair.second.push_back("01:11:00");

    assert(rule.with_interface.has_value());
    assert(rule.with_interface.value() == pair);
  }


  {
    std::cerr <<"[TEST] "<< "Bad interface "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"s5a5\"},"
                         "{\"hash\":\"salkdjlskjf\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00 sal;dfk;}\" }"
                        "]}]}";
    // std::cerr <<json<<std::endl;                   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();  
    try{                 
      guard::GuardRule rule(ptr_obj);
    }
    catch(const std::logic_error& ex){
         assert(std::string(ex.what()) == "Cant parse rule string");
    }
   
  }

  {
    std::cerr <<"[TEST] "<< "With vidpid and interface "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"s5a5\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" }"
                        "]}]}";
    // std::cerr <<json<<std::endl;                   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::GuardRule rule(ptr_obj);
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "s5a5");

    std::pair<guard::RuleOperator,std::vector<std::string>> pair=std::make_pair(guard::RuleOperator::equals,std::vector<std::string>());
    pair.second.push_back("04:00:*");
    pair.second.push_back("01:11:00");

    assert(rule.with_interface.has_value());
    assert(rule.with_interface.value() == pair);
    assert(rule.level == guard::StrictnessLevel::vid_pid);
  }

  
  {
    std::cerr <<"[TEST] "<< "With only an interface "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"block\",\""
                        "fields_arr\":["
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" }"
                        "]}]}";
    // std::cerr <<json<<std::endl;                   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::GuardRule rule(ptr_obj);
    
    assert(rule.target == guard::Target::block);


    assert(rule.with_interface.has_value());
    assert(rule.level == guard::StrictnessLevel::interface);
  }

   {
    std::cerr <<"[TEST] "<< "With only an interface "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"block\",\""
                        "fields_arr\":["
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" }"
                        "]}]}";
    // std::cerr <<json<<std::endl;                   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::GuardRule rule(ptr_obj);
    
    assert(rule.target == guard::Target::block);


    assert(rule.with_interface.has_value());
    assert(rule.level == guard::StrictnessLevel::interface);
  }

  {
    std::cerr <<"[TEST] "<< "With vidpid, interface, ports "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"s5a5\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" },"
                         "{\"via-port\":\"none-of {usb1 usb2 usb3}\"}"
                        "]}]}";
    // std::cerr <<json<<std::endl;                   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::GuardRule rule(ptr_obj);
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "s5a5");

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
    std::cerr <<"[TEST] "<< "With vidpid, interface, ports,conn type "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"s5a5\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" },"
                         "{\"via-port\":\"none-of {usb1 usb2 usb3}\"},"
                         "{\"with-connect-type\":\"hotplug\"}"
                        "]}]}";
    // std::cerr <<json<<std::endl;                   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::GuardRule rule(ptr_obj);
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "s5a5");

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
    std::cerr <<"[TEST] "<< "With vidpid, interface, ports,conn type, conditions "<<std::endl;
    std::string json="{\"deleted_rules\":null,"
                      "\"appended_rules\":"
                      "[{\"table_id\":"
                        "\"list_vidpid_rules\","
                        "\"target\":\"allow\",\""
                        "fields_arr\":"
                        "[{\"vid\":\"a000\"},"
                         "{\"pid\":\"s5a5\"},"
                         "{\"with_interface\":\"{04:00:* 01:11:00}\" },"
                         "{\"via-port\":\"none-of {usb1 usb2 usb3}\"},"
                         "{\"with-connect-type\":\"hotplug\"},"
                         "{\"cond\":\"if one-of {!localtime(00:00:00) true}\"}"
                        "]}]}";
    // std::cerr <<json<<std::endl;                   
    const boost::json::value jv= boost::json::parse(json);                    
    const boost::json::object* ptr_obj= jv.as_object().at("appended_rules").as_array().at(0).if_object();                   
    guard::GuardRule rule(ptr_obj);
    
    assert(rule.target == guard::Target::allow);
    assert(rule.vid == "a000");
    assert(rule.pid == "s5a5");

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
  


  std::cerr << "[TEST] TEST 14 ... OK"<<std::endl;
}