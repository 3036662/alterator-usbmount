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
add_executable(test fstab_test.cpp)
target_link_libraries(test PRIVATE PkgConfig::UDEV)
target_include_directories(test PUBLIC ${UDEV_INCLUDE_DIRS})


# docs

find_package(Doxygen)
if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")
    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
    message ("Doxygen need to be installed to generate documentation.")
endif (DOXYGEN_FOUND)