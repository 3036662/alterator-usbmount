#ifndef LISPMESSAGE_HPP
#define LISPMESSAGE_HPP

#include <string>
#include <map>
#include <iostream>

class LispMessage
{
public:
    LispMessage();

    std::string action;
    std::string objects;
    std::map<std::string,std::string> params;
};

std::ostream& operator  << (std::ostream& os,const LispMessage& mes);

#endif // LISPMESSAGE_HPP
