cmake_minimum_required(VERSION 3.5)

project(backendcpp LANGUAGES CXX)


find_package(PkgConfig REQUIRED)
pkg_check_modules(UDBGUARD REQUIRED IMPORTED_TARGET libusbguard)
message("UDBGUARD_FOUND: ${UDBGUARD_FOUND}")

find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -pedantic)

add_subdirectory(backend)


include(GNUInstallDirs)
