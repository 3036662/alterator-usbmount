#include "guard.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

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
      std::pair<std::string, std::string>{file6, "usb_rule"}}; //even if only authorized
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