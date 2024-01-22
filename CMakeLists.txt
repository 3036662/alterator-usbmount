cmake_minimum_required(VERSION 3.5)

project(backendcpp LANGUAGES CXX)


find_package(PkgConfig REQUIRED)
pkg_check_modules(UDBGUARD REQUIRED IMPORTED_TARGET libusbguard)
message("UDBGUARD_FOUND: ${UDBGUARD_FOUND}")

find_package(Boost REQUIRED)

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

target_link_libraries(simple PRIVATE PkgConfig::UDBGUARD)


include(GNUInstallDirs)
install(TARGETS simple
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION "/home/oleg/dev/alterator/alterator-simple/backend3"
)
