
add_executable(test
               run.cpp 
               test.cpp 
               ../backend/utils.cpp
               ../backend/config_status.cpp
               ../backend/guard.cpp)
target_include_directories(test PRIVATE ../backend)
target_include_directories(test PRIVATE .)
include_directories(test PUBLIC ${USBGUARD_INCLUDE_DIRS})
target_link_libraries(test PRIVATE PkgConfig::USBGUARD)
target_link_libraries(test PRIVATE SDBusCpp::sdbus-c++)
