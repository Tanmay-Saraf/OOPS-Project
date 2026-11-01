cmake_minimum_required(VERSION 3.20)

project(OOPS_Project VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
include(GoogleTest)

# ============================================================
# Networking Library
# ============================================================

add_library(request_parser
    src/network/RequestParser.cpp
    src/network/Server.cpp
    src/network/UniqueFd.cpp
    src/network/ResponseSerializer.cpp
    src/network/ThreadPool.cpp
    src/network/TemporaryMemoryEngine.cpp
)

target_include_directories(request_parser
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(request_parser
    PUBLIC
        Threads::Threads
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

set_target_properties(security_engine PROPERTIES
    EXPORT_NAME security_engine
)

target_include_directories(security_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/security
)

target_link_libraries(security_engine
    PUBLIC
        Threads::Threads
)

# ============================================================
# Security Guard Adapter
# ============================================================

# The adapter connects the networking RequestGuard interface
# to the Security Engine's SecurityDecision interface.
add_library(security_guard_adapter INTERFACE)

target_include_directories(security_guard_adapter
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(security_guard_adapter
    INTERFACE
        request_parser
        oops::security_engine
)

# ============================================================
# Networking Tests
# ============================================================

add_executable(test_parses
    tests/network/test_parser.cpp
)

add_executable(test_response_serializer
    tests/network/test_response_serializer.cpp
)

target_link_libraries(test_response_serializer
    PRIVATE
        request_parser
        GTest::gtest_main
)

gtest_discover_tests(test_response_serializer)

target_link_libraries(test_parses
    PRIVATE
        request_parser
        GTest::gtest_main
)

gtest_discover_tests(test_parses)

add_executable(test_security_guard_adapter
    tests/integration/test_security_guard_adapter.cpp
)

target_link_libraries(test_security_guard_adapter
    PRIVATE
        security_guard_adapter
        GTest::gtest_main
)

gtest_discover_tests(test_security_guard_adapter)

# ============================================================
# TCP Server Executable
# ============================================================

add_executable(server_test
    src/network/server_main.cpp
)

target_link_libraries(server_test
    PRIVATE
        request_parser
        security_guard_adapter
)

target_compile_definitions(server_test
    PRIVATE
        OOPS_WITH_SECURITY
)

# ============================================================
# Security Engine Tests
# ============================================================

function(add_security_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE oops::security_engine)
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

add_executable(SecurityEngineExample
    examples/security_engine_example.cpp
)
target_link_libraries(SecurityEngineExample
    PRIVATE
        oops::security_engine
)
add_test(NAME SecurityEngineExampleSmokeTest
    COMMAND SecurityEngineExample
)

# ============================================================
# Installation
# ============================================================

install(TARGETS security_engine
    EXPORT OOPSSecurityTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES include/common/Request.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/common
)

install(FILES
    include/security/SecurityEngine.hpp
    include/security/SecurityEngine.h
    include/security/SecurityState.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/security
)

install(EXPORT OOPSSecurityTargets
    FILE OOPSSecurityTargets.cmake
    NAMESPACE oops::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OOPSProject
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/OOPSProjectConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/OOPSProjectConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OOPSProject
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/OOPSProjectConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/OOPSProjectConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/OOPSProjectConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OOPSProject
)
