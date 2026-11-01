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
# Memory Engine
# ============================================================

set(MEMORY_ENGINE_SOURCES
    src/memory/VersionChain.cpp
    src/memory/MemoryStore.cpp
    src/memory/MemoryEngine.cpp
)

# Include Version.cpp when present.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/memory/Version.cpp")
    list(APPEND MEMORY_ENGINE_SOURCES
        src/memory/Version.cpp
    )
endif()

add_library(memory_engine
    ${MEMORY_ENGINE_SOURCES}
)

add_library(oops::memory_engine ALIAS memory_engine)

target_include_directories(memory_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(memory_engine
    PUBLIC
        Threads::Threads
)


# ============================================================
# Disk Storage and Recovery
# ============================================================

# The WAL implementation uses POSIX APIs such as open(),
# write() and fdatasync(). Build this target on UNIX/Linux.
if(UNIX)

    set(STORAGE_SOURCES
        src/storage/serializer.cpp
        src/storage/wal_writer.cpp
        src/storage/wal_reader.cpp
        src/storage/recovery_manager.cpp
        src/storage/memory_replay_adapter.cpp
    )

    add_library(storage_engine
        ${STORAGE_SOURCES}
    )

    add_library(storage::engine ALIAS storage_engine)

    target_include_directories(storage_engine
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include/storage
    )

    target_link_libraries(storage_engine
        PUBLIC
            oops::memory_engine
            Threads::Threads
    )

else()

    message(WARNING
        "Disk Storage uses POSIX APIs and will not be built "
        "on native Windows. Use Linux or WSL."
    )

endif()


# ============================================================
# Compiler Warnings
# ============================================================

if(MSVC)

    target_compile_options(memory_engine PRIVATE /W4)

    if(TARGET storage_engine)
        target_compile_options(storage_engine PRIVATE /W4)
    endif()

else()

    target_compile_options(memory_engine
        PRIVATE
            -Wall -Wextra -Wpedantic
    )

    if(TARGET storage_engine)
        target_compile_options(storage_engine
            PRIVATE
                -Wall -Wextra -Wpedantic
        )
    endif()

endif()


# ============================================================
# Security Guard Adapter
# ============================================================

# Connects the networking RequestGuard interface
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

target_link_libraries(test_parses
    PRIVATE
        request_parser
        GTest::gtest_main
)

gtest_discover_tests(test_parses)


add_executable(test_response_serializer
    tests/network/test_response_serializer.cpp
)

target_link_libraries(test_response_serializer
    PRIVATE
        request_parser
        GTest::gtest_main
)

gtest_discover_tests(test_response_serializer)


# ============================================================
# Security Guard Adapter Tests
# ============================================================

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
        oops::memory_engine
)

# Storage is available on Linux/UNIX.
if(TARGET storage_engine)
    target_link_libraries(server_test
        PRIVATE
            storage::engine
    )
endif()

target_compile_definitions(server_test
    PRIVATE
        OOPS_WITH_SECURITY
)


# ============================================================
# Security Engine Tests
# ============================================================

function(add_security_test test_name test_source)

    add_executable(${test_name}
        ${test_source}
    )

    target_link_libraries(${test_name}
        PRIVATE
            oops::security_engine
    )

    add_test(
        NAME ${test_name}
        COMMAND ${test_name}
    )

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


# ============================================================
# Security Engine Example
# ============================================================

add_executable(SecurityEngineExample
    examples/security_engine_example.cpp
)

target_link_libraries(SecurityEngineExample
    PRIVATE
        oops::security_engine
)

add_test(
    NAME SecurityEngineExampleSmokeTest
    COMMAND SecurityEngineExample
)


# ============================================================
# Memory and Storage Tests
# ============================================================

if(BUILD_TESTING)

    # -----------------------------
    # Memory Engine
    # -----------------------------

    add_executable(memory_store_test
        tests/memory/memory_store_test.cpp
    )

    target_link_libraries(memory_store_test
        PRIVATE
            oops::memory_engine
            Threads::Threads
    )

    add_test(
        NAME memory_store_test
        COMMAND memory_store_test
    )


    # -----------------------------
    # Disk Storage and Recovery
    # -----------------------------

    if(TARGET storage_engine)

        set(STORAGE_TESTS
            serializer_test
            wal_writer_test
            wal_reader_test
            wal_reader_empty_test
            wal_reader_corrupted_checksum_test
            wal_reader_truncated_prefix_test
            wal_reader_truncated_payload_test
        )

        foreach(test_name IN LISTS STORAGE_TESTS)

            add_executable(${test_name}
                tests/storage/${test_name}.cpp
            )

            target_include_directories(${test_name}
                PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}/include/storage
            )

            target_link_libraries(${test_name}
                PRIVATE
                    storage::engine
            )

            add_test(
                NAME ${test_name}
                COMMAND ${test_name}
            )

        endforeach()


        # -----------------------------
        # Memory + Storage Integration
        # -----------------------------

        add_executable(memory_engine_storage_integration_test
            tests/storage/memory_engine_storage_integration_test.cpp
        )

        target_include_directories(memory_engine_storage_integration_test
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/include/storage
        )

        target_link_libraries(memory_engine_storage_integration_test
            PRIVATE
                storage::engine
        )

        add_test(
            NAME memory_engine_storage_integration_test
            COMMAND memory_engine_storage_integration_test
        )

    endif()

endif()


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

install(FILES
    include/common/Request.hpp
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
