add_library(DAL OBJECT
    local_storage.cpp
    dto.cpp
)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_target(pre_build DEPENDS $<TARGET_OBJECTS:DAL>)
    include(Format)
    Format(pre_build .)
    include(ClangTidy)
    AddClangTidy(DAL)
    add_dependencies(DAL run_cppcheck)
endif()