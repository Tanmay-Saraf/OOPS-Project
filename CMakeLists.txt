
cmake_minimum_required(VERSION 3.20)

project(OOPS_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable CTest
include(CTest)

# Find GoogleTest
find_package(GTest REQUIRED)

# Build the networking library
add_library(request_parser
    src/network/RequestParser.cpp
    src/network/Server.cpp
    src/network/UniqueFd.cpp
    src/network/ResponseSerializer.cpp
)

target_include_directories(request_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Build the parser tests
add_executable(test_parses
    tests/network/test_parser.cpp
)

target_link_libraries(test_parses PRIVATE
    request_parser
    GTest::gtest_main
)

# Temporary executable for testing the TCP server
add_executable(server_test
    src/network/server_main.cpp
)

target_link_libraries(server_test PRIVATE
    request_parser
)

# Discover and register individual GoogleTests
include(GoogleTest)
gtest_discover_tests(test_parses)
