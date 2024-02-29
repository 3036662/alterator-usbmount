function(AddCppCheck target)
find_program(CPPCHECK_EXECUTABLE cppcheck REQUIRED)
if(CPPCHECK_EXECUTABLE)
    add_custom_target(run_cppcheck
        COMMAND ${CPPCHECK_EXECUTABLE} --enable=all  --check-level=exhaustive --inconclusive --force --inline-suppr --template=gcc --std=c++17 --suppressions-list=${CMAKE_SOURCE_DIR}/CppCheckSuppressions.cppcheck ${target}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck on all files in folder"
    )
else()
    message(WARNING "cppcheck not found. Please install cppcheck to enable static code analysis.")
endif()

endfunction()