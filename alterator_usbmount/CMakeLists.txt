add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/backend)


install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/usbmount_ui/USBMount.desktop
    DESTINATION "/usr/share/alterator/applications/"
)

install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/usbmount_ui/ajax.scm 
    ${CMAKE_CURRENT_SOURCE_DIR}/usbmount_ui/index.html 
    DESTINATION "/usr/share/alterator/ui/usbmount"
)

install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/usbmount_ui/alt_usb_mount.css
    DESTINATION "/usr/share/alterator/design/styles"
)