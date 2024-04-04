
#include <exception>
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>
#include <string>

int main(int argc, char *argv[]) {
  if (argc < 2)
    return 0;
  std::string dev = argv[1];
  if (dev.empty()) {
    std::cout << "EMPTY ARGS";
    return 0;
  }
  const std::string dest = "ru.alterator.usbd";
  const std::string object_path = "/ru/alterator/altusbd";
  const std::string interface_name = "ru.alterator.Usbd";
  try {
    auto proxy = sdbus::createProxy(dest, object_path);
    auto method =
        proxy->createMethodCall(interface_name, "CanAnotherUserUnmount");
    method << dev;
    auto reply = proxy->callMethod(method);
    std::string res;
    reply >> res;
    std::cout << res;
  } catch (const std::exception &ex) {
    std::cout << "Exception";
    return 0;
  }

  return 0;
}