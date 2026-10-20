cmake_minimum_required(VERSION 3.20)

project(OOPS_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

# Dependencies
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# ============================================================
# Networking Library
# ============================================================

add_library(request_parser
    src/network/RequestParser.cpp
    src/network/Server.cpp
    src/network/UniqueFd.cpp
    src/network/ResponseSerializer.cpp
    src/network/ThreadPool.cpp
)

target_include_directories(request_parser
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ============================================================
# Security Engine Library
# ============================================================

set(OOPS_SECURITY_DIR "" CACHE PATH "Path to a Security_Defense checkout for local integration builds")

if(OOPS_SECURITY_DIR)

    add_library(security_engine
        ${OOPS_SECURITY_DIR}/src/security/ModificationActivityDetector.cpp
        ${OOPS_SECURITY_DIR}/src/security/CanaryDetector.cpp
        ${OOPS_SECURITY_DIR}/src/security/EntropyDetector.cpp
        ${OOPS_SECURITY_DIR}/src/security/SecurityEngine.cpp
        ${OOPS_SECURITY_DIR}/src/security/SecurityLogger.cpp
        ${OOPS_SECURITY_DIR}/src/security/StateManager.cpp
    )

    add_library(oops::security_engine ALIAS security_engine)

    target_include_directories(security_engine
        PUBLIC
            ${OOPS_SECURITY_DIR}/include
    )

    target_link_libraries(security_engine
        PUBLIC
            Threads::Threads
    )

endif()

# ============================================================
# Networking Tests
# ============================================================

add_executable(test_parses
    tests/network/test_parser.cpp
)

target_link_libraries(test_parses
    PRIVATE
        request_parser
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_parses)

# ============================================================
# TCP Server Executable
# ============================================================

add_executable(server_test
    src/network/server_main.cpp
)

target_link_libraries(server_test
    PRIVATE
        request_parser
)

if(OOPS_SECURITY_DIR)

    target_link_libraries(server_test
        PRIVATE
            oops::security_engine
    )

    target_include_directories(server_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_definitions(server_test
        PRIVATE
            OOPS_WITH_SECURITY
    )

endif()
