add_executable(altusbd 
    main.cpp
    daemon.cpp
    udev_monitor.cpp
    udisks_dbus.cpp
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


install(TARGETS altusbd
    RUNTIME DESTINATION "/usr/bin"
)

install(FILES 
    ${CMAKE_SOURCE_DIR}/daemon/altusbd.service
    DESTINATION "/lib/systemd/system/"
)

install(FILES 
    ${CMAKE_SOURCE_DIR}/daemon/ru.alterator.usbd.conf
    DESTINATION "/usr/share/dbus-1/system.d/"
)
