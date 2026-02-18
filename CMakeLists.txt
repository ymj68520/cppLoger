cmake_minimum_required(VERSION 3.16)
project(CppLogger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" ON)

# Header-only library
add_library(logger INTERFACE)
target_include_directories(logger INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Demo program
add_executable(cpp_logger_demo main.cpp)
target_link_libraries(cpp_logger_demo PRIVATE logger)

# Tests subdirectory
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
