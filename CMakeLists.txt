cmake_minimum_required(VERSION 3.5)

project(backendcpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -pedantic)

add_executable(simple main.cpp
    lispmessage.hpp lispmessage.cpp
    message_reader.hpp message_reader.cpp
    message_dispatcher.cpp message_dispatcher.hpp
    utils.cpp utils.hpp
    usb_device.cpp usb_device.hpp
    )

include(GNUInstallDirs)
install(TARGETS simple
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION "/home/oleg/dev/alterator/alterator-simple/backend3"
)
