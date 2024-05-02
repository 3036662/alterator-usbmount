add_library(boost_json OBJECT boost_json.cpp)

add_library(systemd_dbus OBJECT systemd_dbus.cpp )

target_include_directories(systemd_dbus PRIVATE ${CMAKE_SOURCE_DIR}/alterator_bindings)
