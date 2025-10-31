cmake_minimum_required(VERSION 3.10)

# Project name
project(subzero VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard - use C++98 for Atari compatibility
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add option for gnu++0x if available
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Try to use gnu++0x if supported, fallback to gnu++98
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-std=gnu++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++98")
    endif()
endif()

# Add include directory
include_directories(include)

# Collect all .cpp files in src directory
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Platform-specific libraries and settings
if(WIN32)
    # Windows console API
    target_compile_definitions(${PROJECT_NAME} PRIVATE WINDOWS_PLATFORM)
    # Link with Windows console libraries if needed
    # target_link_libraries(${PROJECT_NAME} PRIVATE kernel32 user32)
elseif(UNIX AND NOT APPLE)
    # Linux - use ncurses (no need for dl library anymore)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NCURSES REQUIRED ncurses)
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE LINUX_PLATFORM)
    target_include_directories(${PROJECT_NAME} PRIVATE ${NCURSES_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${NCURSES_LIBRARIES})
    target_compile_options(${PROJECT_NAME} PRIVATE ${NCURSES_CFLAGS_OTHER})
elseif(APPLE)
    # macOS - use ncurses (usually pre-installed)
    find_library(NCURSES_LIBRARY ncurses)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MACOS_PLATFORM)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${NCURSES_LIBRARY})
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O2)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /O2)
endif()

# Set output directory to project root
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Print build information
message(STATUS "Building ${PROJECT_NAME} version ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source files found: ${SOURCES}")