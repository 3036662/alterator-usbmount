#include "test.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  std::cout << "Tests..." << std::endl;
  Test test;
  test.Run1();
  // test2 passes only if usbguard is active and service is enabled
  test.Run2();
  test.Run3();
  return 0;
}