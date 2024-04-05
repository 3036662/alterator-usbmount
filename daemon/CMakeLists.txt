add_executable(altusbd 
    main.cpp
    daemon.cpp
    udev_monitor.cpp
   # udisks_dbus.cpp 
    utils.cpp
    usb_udev_device.cpp
    custom_mount.cpp
    dbus_methods.cpp
    local_storage.cpp
)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(altusbd .)
    include(ClangTidy)
    AddClangTidy(altusbd)
    add_dependencies(altusbd run_cppcheck)
endif()

include_directories(altusbd PUBLIC ${SYSTEMD_INCLUDE_DIRS})
target_link_libraries(altusbd PRIVATE PkgConfig::SYSTEMD)
target_link_libraries(altusbd PRIVATE SDBusCpp::sdbus-c++)

target_link_libraries(altusbd PRIVATE PkgConfig::UDEV)
target_include_directories(altusbd PUBLIC ${UDEV_INCLUDE_DIRS})

# acl 
pkg_check_modules(ACL REQUIRED IMPORTED_TARGET libacl)
target_link_libraries(altusbd PRIVATE PkgConfig::ACL)
target_include_directories(altusbd PUBLIC ${ACL_INCLUDE_DIRS})

# logger
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
target_link_libraries(altusbd PRIVATE fmt)


install(TARGETS altusbd
    RUNTIME DESTINATION "/usr/bin"
)

install(FILES 
    ${CMAKE_SOURCE_DIR}/daemon/service_files/altusbd.service
    DESTINATION "/lib/systemd/system/"
)

install(FILES 
    ${CMAKE_SOURCE_DIR}/daemon/service_files/ru.alterator.usbd.conf
    DESTINATION "/usr/share/dbus-1/system.d/"
)

add_subdirectory(polkit)
