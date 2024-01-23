#include <iostream>
#include <string>
#include <cstdio>
#include <memory>



#include "lispmessage.hpp"
#include "message_reader.hpp"
#include "guard.hpp"


int main(int argc, char* argv[], char **envp )
{

   // std::shared_ptr<Guard> ptr_guard=std::make_shared<Guard>();
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
