cmake_minimum_required(VERSION 3.16)

project(OOPS_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(security_engine
    src/security/ModificationActivityDetector.cpp
    src/security/CanaryDetector.cpp
    src/security/SecurityEngine.cpp
    src/security/SecurityLogger.cpp
    src/security/StateManager.cpp
)

target_include_directories(security_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

function(add_security_test test_name test_source)
    add_executable(${test_name}
        ${test_source}
    )

    target_link_libraries(${test_name}
        PRIVATE security_engine
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_security_test(
    ModificationActivityDetectorTest
    tests/ModificationActivityDetectorTest.cpp
)

add_security_test(
    CanaryDetectorTest
    tests/CanaryDetectorTest.cpp
)

add_security_test(
    CanarySecurityEngineTest
    tests/CanarySecurityEngineTest.cpp
)

add_security_test(
    SecurityEngineTest
    tests/SecurityEngineTest.cpp
)

add_security_test(
    SecurityLoggerTest
    tests/SecurityLoggerTest.cpp
)

add_security_test(
    StateManagerTest
    tests/StateManagerTest.cpp
)
