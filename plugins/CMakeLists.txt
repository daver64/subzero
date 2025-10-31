# C/C++ Syntax Highlighter Plugin
add_library(cpp_highlighter SHARED cpp_highlighter.cpp)

# Include directories
target_include_directories(cpp_highlighter PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Set output directory to plugins folder in project root
set_target_properties(cpp_highlighter PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/plugins
)

# Platform-specific settings
if(WIN32)
    # Windows: Create .dll
    set_target_properties(cpp_highlighter PROPERTIES
        SUFFIX ".dll"
    )
else()
    # Linux: Create .so
    set_target_properties(cpp_highlighter PROPERTIES
        SUFFIX ".so"
    )
endif()

# Compiler settings for the plugin
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cpp_highlighter PRIVATE -fPIC)
endif()