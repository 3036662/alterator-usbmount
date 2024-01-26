cmake_minimum_required(VERSION 3.5)

project(backendcpp LANGUAGES CXX)


find_package(PkgConfig REQUIRED)
pkg_check_modules(USBGUARD REQUIRED IMPORTED_TARGET libusbguard)
message("USBGUARD_FOUND: ${USBGUARD_FOUND}")

find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -pedantic)

add_subdirectory(backend)

include(GNUInstallDirs)


pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)
add_executable(test main.cpp)
target_link_libraries(test PRIVATE PkgConfig::UDEV)
target_include_directories(test PUBLIC ${UDEV_INCLUDE_DIRS})