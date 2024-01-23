#ifndef MESSAGE_READER_HPP
#define MESSAGE_READER_HPP

#include <string>
#include "message_dispatcher.hpp"
#include "guard.hpp"
class MessageReader
{
public:
    MessageReader(Guard& guard);
    
    //main loop
    void Loop();

private:
    MessageDispatcher dispatcher;

    const std::string str_action="action:";
    const std::string str_objects="_objects:";

};

#endif // MESSAGE_READER_HPP
