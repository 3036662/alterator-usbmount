add_library(boost_json OBJECT boost_json.cpp)

add_library(systemd_dbus OBJECT systemd_dbus.cpp )

add_library(log_reader OBJECT log_reader.cpp)

target_include_directories(systemd_dbus PRIVATE ${CMAKE_SOURCE_DIR}/alterator_bindings)

target_include_directories(log_reader PRIVATE ${CMAKE_SOURCE_DIR}/alterator_bindings)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
include(Format)
FormatDir(log_reader_pre .)
add_dependencies(log_reader log_reader_pre)

include(ClangTidy)
AddClangTidy(log_reader)
AddClangTidy(systemd_dbus)

endif()

