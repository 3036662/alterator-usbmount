
add_executable(simple main.cpp
    lispmessage.hpp lispmessage.cpp
    message_reader.hpp message_reader.cpp
    message_dispatcher.cpp message_dispatcher.hpp
    utils.cpp utils.hpp
    usb_device.cpp usb_device.hpp
    guard.cpp guard.hpp
    )

include_directories(simple PUBLIC ${USBGUARD_INCLUDE_DIRS})

include_directories(simple PUBLIC $CMAKE_SOURCE_DIR/tests) 

target_link_libraries(simple PRIVATE PkgConfig::USBGUARD)

install(TARGETS simple
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION "/usr/lib/alterator/backend3"
)

install(FILES alterator_ui/ajax.scm alterator_ui/index.html 
        DESTINATION "/usr/share/alterator/ui"
)

install(CODE "execute_process(COMMAND service alteratord restart)")
install(CODE "execute_process(COMMAND service ahttpd restart)")