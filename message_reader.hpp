#ifndef MESSAGE_READER_HPP
#define MESSAGE_READER_HPP

#include <string>

class MessageReader
{
public:
    MessageReader() =default;
    
    //main loop
    void Loop();

private:
    const std::string str_action="action:";
    const std::string begining="(\n";
    const std::string ending=")\n";
    const std::string str_objects="_objects:";

    std::string WrapWithQuotes(const std::string& str);


};

#endif // MESSAGE_READER_HPP
