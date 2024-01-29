#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "guard.hpp"
#include "lispmessage.hpp"
#include "message_reader.hpp"

int main(int argc, char *argv[], char **envp) {
  Guard guard;

  MessageReader reader(guard);
  reader.Loop();
  return 0;
}
