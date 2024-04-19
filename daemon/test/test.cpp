#define CATCH_CONFIG_MAIN
#include "utils.hpp"
#include <catch2/catch.hpp>

TEST_CASE("Test utils") {
  using namespace usbmount::utils;
  SECTION("Test /etc/login.defs parsing") {
    auto res =
        GetSystemUidMinMax(InitLogFile("/var/log/alt-usb-automount/log.txt"));
    REQUIRE(res.has_value());
    REQUIRE(res->uid_min > 0);
    REQUIRE(res->uid_max > 0);
    REQUIRE(res->gid_min > 0);
    REQUIRE(res->gid_max > 0);
  }

  SECTION("string to uint64") {
    REQUIRE(StrToUint("12984798") == 12984798);
    REQUIRE(StrToUint("012984798") == 12984798);
    REQUIRE(StrToUint("000000012984798") == 12984798);
    REQUIRE_THROWS(StrToUint(""));
    REQUIRE_THROWS(StrToUint("12984798d"));
    REQUIRE_THROWS(StrToUint("-12984798"));
    REQUIRE_THROWS(StrToUint("12-984798"));
  }

  SECTION("read possible shells") {
    auto shells =
        GetPossibleShells(InitLogFile("/var/log/alt-usb-automount/log.txt"));
    REQUIRE(!shells.empty());
    for (const auto &shell : shells) {      
      REQUIRE(!shell.empty());
      REQUIRE(shell[0]=='/');
    }
  }
}