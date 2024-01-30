
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
    RUNTIME DESTINATION "/usr/lib/alterator/backend3"
)

install(FILES 
        ${CMAKE_SOURCE_DIR}/alterator_ui/ajax.scm 
        ${CMAKE_SOURCE_DIR}/alterator_ui/index.html 
        DESTINATION "/usr/share/alterator/ui/simple"
)

install(CODE "execute_process(COMMAND service alteratord restart)")
install(CODE "execute_process(COMMAND service ahttpd restart)")