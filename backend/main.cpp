#include "guard.hpp"
#include "message_reader.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  guard::Guard guard;
  MessageReader reader(guard);
  reader.Loop();
  return 0;
}
