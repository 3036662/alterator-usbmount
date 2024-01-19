#ifndef LISPMESSAGE_HPP
#define LISPMESSAGE_HPP

#include <string>
#include <unordered_map>
#include <iostream>

class LispMessage
{
public:
    LispMessage();
    LispMessage(const std::string& act,
                const std::string& obj,
                const std::unordered_map<std::string,std::string>& prms);

    std::string action;
    std::string objects;
    std::unordered_map<std::string,std::string> params;
};

std::ostream& operator  << (std::ostream& os,const LispMessage& mes);

#endif // LISPMESSAGE_HPP
