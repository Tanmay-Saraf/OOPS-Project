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

add_library(security_engine
    src/security/ModificationActivityDetector.cpp
    src/security/CanaryDetector.cpp
    src/security/EntropyDetector.cpp
    src/security/SecurityEngine.cpp
    src/security/SecurityLogger.cpp
    src/security/StateManager.cpp
)

add_library(oops::security_engine ALIAS security_engine)

target_include_directories(security_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(security_engine
    PUBLIC
        Threads::Threads
)

if(MSVC)
    target_compile_options(security_engine PRIVATE /W4)
else()
    target_compile_options(security_engine PRIVATE
        -Wall -Wextra -Wpedantic
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
        oops::security_engine
)

# ============================================================
# Security Engine Tests
# ============================================================

function(add_security_test test_name test_source)
    add_executable(${test_name} ${test_source})

    target_link_libraries(${test_name}
        PRIVATE
            oops::security_engine
    )

    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4)
    else()
        target_compile_options(${test_name} PRIVATE
            -Wall -Wextra -Wpedantic
        )
    endif()

    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_security_test(CanaryDetectorTest
    tests/CanaryDetectorTest.cpp
)

add_security_test(CanarySecurityEngineTest
    tests/CanarySecurityEngineTest.cpp
)

add_security_test(ModificationActivityDetectorTest
    tests/ModificationActivityDetectorTest.cpp
)

add_security_test(SecurityEngineTest
    tests/SecurityEngineTest.cpp
)

add_security_test(SecurityLoggerTest
    tests/SecurityLoggerTest.cpp
)

add_security_test(StateManagerTest
    tests/StateManagerTest.cpp
)

add_security_test(EntropyDetectorTest
    tests/EntropyDetectorTest.cpp
)

add_security_test(SecurityPublicApiTest
    tests/SecurityPublicApiTest.cpp
)

add_security_test(SecurityLockdownPolicyTest
    tests/SecurityLockdownPolicyTest.cpp
)

add_security_test(SecurityCommandPolicyTest
    tests/SecurityCommandPolicyTest.cpp
)
