#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "guard.hpp"
#include "lispmessage.hpp"
#include "message_reader.hpp"

int main([[maybe_unused]] int argc,[[maybe_unused]]  char *argv[],[[maybe_unused]]  char **envp) {
  guard::Guard guard;

  MessageReader reader(guard);
  reader.Loop();
  return 0;
}
