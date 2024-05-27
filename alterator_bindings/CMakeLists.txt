add_library(lisp_bindings OBJECT 
    lisp_message.cpp
    common_utils.cpp
    message_reader.cpp
    message_dispatcher.cpp
)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
   include(Format)
   FormatDir(lisp_bindings_pre .)
   add_dependencies(lisp_bindings lisp_bindings_pre)
   include(ClangTidy)
   include(CppCheck)
   AddClangTidy(lisp_bindings)
   AddCppCheck(lisp_bindings_cpp_check)
   add_dependencies(lisp_bindings lisp_bindings_cpp_check)
endif()