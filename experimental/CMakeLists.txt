# fstab test
add_executable(test_fstab fstab_test.cpp)
target_link_libraries(test_fstab PRIVATE PkgConfig::UDEV)
target_include_directories(test_fstab PUBLIC ${UDEV_INCLUDE_DIRS})    
# the automount
add_subdirectory(automount)