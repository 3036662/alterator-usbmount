add_executable(usbmount
    main.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
include(Format)
Format(usbmount .)
include(ClangTidy)
AddClangTidy(usbmount)
AddCppCheck(usbmount_backend_cppcheck)
add_dependencies(usbmount usbmount_backend_cppcheck)
endif()