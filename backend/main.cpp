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

  // for (char **env = envp; *env != 0; env++)
  // {
  //     char *thisEnv = *env;
  //     std::cerr << thisEnv <<std::endl;
  // }
  return 0;
}
