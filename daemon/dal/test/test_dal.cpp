#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
#include <future>
#define CATCH_CONFIG_MAIN
#include "dto.hpp"
#include "local_storage.hpp"
#include <boost/json.hpp>
#include <catch2/catch.hpp>
#include <filesystem>
#include <string>
#include <thread>

using namespace usbmount::dal;
namespace fs = std::filesystem;

// clang-format off
TEST_CASE("Test DTO objects"){
  SECTION("Device"){
    {
      Device dev;
      REQUIRE(dev.Serialize()=="{\"vid\":\"\",\"pid\":\"\",\"serial\":\"\"}");      
    }
    {
      Device dev({"aa10","bb10","saodik"});
      REQUIRE(dev.Serialize()=="{\"vid\":\"aa10\",\"pid\":\"bb10\",\"serial\":\"saodik\"}");      
    }
    {
      REQUIRE_THROWS(
        Device({"xx","bb10","sdfsdfsdf"})
      );      
    }
    {
      boost::json::value val=boost::json::parse("{\"vIIIid\":\"aa10\",\"pid\":\"bb10\",\"serial\":\"saodik\"}");
      REQUIRE_THROWS(
         Device(val.as_object())
      );      
    }
    {
      boost::json::value val=boost::json::parse("{\"vid\":\"aa10\",\"pid\":\"bb10\",\"serial\":\"saodik\"}");
      Device dev(val.as_object());
      REQUIRE(dev.Serialize()=="{\"vid\":\"aa10\",\"pid\":\"bb10\",\"serial\":\"saodik\"}");      
    }
  }

  SECTION("User"){
    {
      User user;
      REQUIRE(user.Serialize()=="{\"uid\":0,\"name\":\"\"}");
    }
    {
      REQUIRE_THROWS(
        User(0,"")
      );  
    }
    {
      User user(0,"rooot");
      REQUIRE(user.Serialize()=="{\"uid\":0,\"name\":\"rooot\"}");
    }
     {
       boost::json::value val=boost::json::parse("{\"uid\":0,\"name\":\"rooot\"}");
       User user(val.as_object());
       REQUIRE(user.Serialize()=="{\"uid\":0,\"name\":\"rooot\"}");      
     }


  }
}


TEST_CASE("Test local storage") {
  SECTION("Test storage singleton from different threads") {
    std::future<std::shared_ptr<LocalStorage>> fut1 =
        std::async(std::launch::async, LocalStorage::GetStorage);
    std::future<std::shared_ptr<LocalStorage>> fut2 =
        std::async(std::launch::async, LocalStorage::GetStorage);
    auto storage0 = LocalStorage::GetStorage();
    fut1.wait();
    fut2.wait();
    REQUIRE(fut1.valid());
    REQUIRE(fut2.valid());
    auto storage1 = fut1.get();
    auto storage2 = fut2.get();
    REQUIRE(storage1 == storage2);
    REQUIRE(storage1 == storage0);
    REQUIRE(storage2 == storage0);
  }

  const std::string path_perm = "/var/lib/alt-usb-mount/permissions.json";
  const std::string path_mounts = "/var/lib/alt-usb-mount/mount_points.json";

  SECTION("Check json files existance") {
    LocalStorage::GetStorage();
    REQUIRE(fs::exists(path_mounts));
    REQUIRE(fs::exists(path_perm));
  }

  SECTION("DevicePermissions") {
    auto db=LocalStorage::GetStorage ();
    REQUIRE(fs::exists(path_mounts));
    REQUIRE(fs::exists(path_perm));
    db->permissions.Clear();
    REQUIRE(db->permissions.size()==0);
    REQUIRE(db->permissions.Serialize()=="[]");
     //Device ({}});
   // db->permissions.Create(PermissionEntry{})
  }
};