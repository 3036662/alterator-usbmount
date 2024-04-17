#include "message_reader.hpp"
#include "usb_mount.hpp"
int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  alterator::usbmount::UsbMount mount;
  // MessageReader reader(mount);
  // reader.Loop();
  return 0;
}