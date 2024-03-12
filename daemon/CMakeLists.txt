add_executable(altusbd 
    main.cpp
    daemon.cpp
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
