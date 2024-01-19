#include <iostream>
#include <string>
#include <cstdio>


#include "lispmessage.hpp"
#include "message_reader.hpp"



int main(int argc, char* argv[], char **envp)
{

    std::cerr << "argc = " <<argc <<std::endl;

    std::string line;
    bool msg_in_progress=false;
    std::string action;
    std::string parameter;

    MessageReader reader;
    reader.Loop();


    // for (char **env = envp; *env != 0; env++)
    // {
    //     char *thisEnv = *env;
    //     std::cerr << thisEnv <<std::endl;
    // }
    return 0;
}
