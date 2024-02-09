#include "guard.hpp"
#include "guard_rule.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
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
  str = "allow via-port all-of {" +WrapWithQuotes("1-2")+" "+ WrapWithQuotes("2-2") + "}";
  {
    //std::cerr << "START" << std::endl;
    guard::GuardRule parser(str);
    std::vector<std::string> ports_expected{"\"1-2\"","\"2-2\""};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_port{
        guard::RuleOperator::all_of, ports_expected};

    assert(parser.target == guard::Target::allow && !parser.vid &&
           !parser.pid && !parser.hash && !parser.device_name &&
           !parser.serial && *parser.port == expected_port &&
           !parser.with_interface && !parser.cond);
  }

  str = "allow via-port all-of {" +WrapWithQuotes("1-2")+" "+ WrapWithQuotes("2-2") + "";
  {
    //std::cerr << "START" << std::endl;
    try {
      guard::GuardRule parser(str);
    }
    catch(const std::logic_error& ex){
      std::cerr << "Catched expected exception" << std::endl;
      std::cerr << ex.what()<<std::endl;
    }
  }

   // two ports one interface
  str = "allow via-port all-of {" +WrapWithQuotes("1-2")+" "+ WrapWithQuotes("2-2") + "} with-interface 08:06:50";
  {
    //std::cerr << "START" << std::endl;
    guard::GuardRule parser(str);
    std::vector<std::string> ports_expected{"\"1-2\"","\"2-2\""};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_port{
        guard::RuleOperator::all_of, ports_expected};

    std::vector<std::string> interface_expected={"08:06:50"};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_interface{
        guard::RuleOperator::no_operator, interface_expected};

    assert(parser.target == guard::Target::allow && !parser.vid &&
           !parser.pid && !parser.hash && !parser.device_name &&
           !parser.serial && *parser.port == expected_port &&
           *parser.with_interface == expected_interface && 
           !parser.cond);
  }


   // two ports three interfaces
  str = "allow via-port all-of {" +WrapWithQuotes("1-2")+" "+ WrapWithQuotes("2-2") + "} with-interface none-of{08:06:50 07:*:*}";
  {
    guard::GuardRule parser(str);
    std::vector<std::string> ports_expected{"\"1-2\"","\"2-2\""};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_port{
        guard::RuleOperator::all_of, ports_expected};

    std::vector<std::string> interface_expected={"08:06:50","07:*:*"};
    std::pair<guard::RuleOperator, std::vector<std::string>> expected_interface{
        guard::RuleOperator::none_of, interface_expected};

    assert(parser.target == guard::Target::allow && !parser.vid &&
           !parser.pid && !parser.hash && !parser.device_name &&
           !parser.serial && *parser.port == expected_port &&
           *parser.with_interface == expected_interface && 
           !parser.cond);
  }


// conditioins
std::cerr << "[TEST] START CONDITIONS TEST" <<std::endl;


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



  std::cerr << "TEST10 .... OK" << std::endl;
}