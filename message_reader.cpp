#include "message_reader.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <iostream>
#include <fstream>
#include <unordered_map>


void MessageReader::Loop(){
    std::string line;
    bool msg_in_progress=false;
    std::string action;
    std::unordered_map<std::string,std::string> params;

    // read stdin loop
    while (std::getline(std::cin,line) ) {  
        std::cerr <<line<<std::endl;
        boost::trim(line);
        
        // message begin
        if (boost::contains(line,"_message:begin")){
            msg_in_progress=true;
            line.clear();
            continue;
        }
        // end of messages
        else{
            if (!msg_in_progress) break;
        }
        
        // the end of a message
        if(boost::contains(line,"_message:end")){
            msg_in_progress=false;
            //TODO  create and send messag
            continue;
        }

        // find action
        if (boost::starts_with(line,str_action)){
            boost::erase_first(line,str_action);
            boost::trim(line);
            action=line;
            std::cerr << "Action = " <<action <<std::endl;
            line.clear();
            continue;
        }

        // find parameters
        size_t pos=line.find(':');
        if (pos!=std::string::npos){
            params.emplace(
              line.substr(0,pos), // param
              line.substr(pos+1)  // value
            );
        }

        line.clear();
    }
    
}

std::string MessageReader::WrapWithQuotes(const std::string& str){
    std::string res;
    res+="\"";
    res+=str;
    res+="\"";
    return res;
}


 // if (action == "read"){
            //     // read from file
            //     std::ifstream is("/tmp/config");
            //     if (is.is_open()){
            //         is >> parameter;
            //     }
            //     else{
            //         std::cerr << "Can't open file for reading"<<std::endl;
            //     }
            //     std::cerr <<"Sending response = " << begining<<"parameter "<< WrapWithQuotes(parameter) << ending <<std::endl;
            //     std::cout  << begining<<"parameter "<< WrapWithQuotes(parameter) << ending <<std::endl;
            // }
            // else if (action == "write" && !parameter.empty()){
            //     std::ofstream os("/tmp/config");
            //     if (os.is_open()){
            //         os << parameter;
            //     }
            //     else {
            //         std::cerr << "Can't open file for writing"<<std::endl;
            //     }
            //     os.close();
            //     std::cout << begining <<ending;
            // }
            // else{
            //     // just empty answer for other requests
            //       std::cout << begining <<ending;
            // }