# GoogleTest paths
set(GTEST_INCLUDE_DIR /usr/local/include)
set(GTEST_LIB_DIR /usr/local/lib)

# Create test executable
add_executable(logger_tests
    main_test.cpp
    test_singleton.cpp
    test_log_level.cpp
    test_console_output.cpp
    test_file_output.cpp
    test_log_stream.cpp
    test_thread_safety.cpp
    test_edge_cases.cpp
    test_performance.cpp
)

target_include_directories(logger_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIR}
)

target_link_libraries(logger_tests
    ${GTEST_LIB_DIR}/libgtest.a
    ${GTEST_LIB_DIR}/libgtest_main.a
    pthread
)

# Enable C++20
target_compile_features(logger_tests PRIVATE cxx_std_20)

# Register tests
include(GoogleTest)
gtest_discover_tests(logger_tests)
