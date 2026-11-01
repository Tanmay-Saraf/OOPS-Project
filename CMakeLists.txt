cmake_minimum_required(VERSION 3.20)

project(KittyDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
find_package(Threads REQUIRED)

# -----------------------------
# Memory Engine
# -----------------------------
set(MEMORY_ENGINE_SOURCES
    src/memory/VersionChain.cpp
    src/memory/MemoryStore.cpp
    src/memory/MemoryEngine.cpp
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/memory/Version.cpp")
    list(APPEND MEMORY_ENGINE_SOURCES src/memory/Version.cpp)
endif()

add_library(memory_engine ${MEMORY_ENGINE_SOURCES})
add_library(oops::memory_engine ALIAS memory_engine)

target_include_directories(memory_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(memory_engine PUBLIC Threads::Threads)

# -----------------------------
# Disk Storage and Recovery
# -----------------------------
# The supplied WAL writer uses POSIX open/write/fdatasync APIs.
# Build the Storage target on Linux/Unix (including WSL).
if(UNIX)
    set(STORAGE_SOURCES
        src/storage/serializer.cpp
        src/storage/wal_writer.cpp
        src/storage/wal_reader.cpp
        src/storage/recovery_manager.cpp
        src/storage/memory_replay_adapter.cpp
    )

    add_library(storage_engine ${STORAGE_SOURCES})
    add_library(storage::engine ALIAS storage_engine)

    target_include_directories(storage_engine
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/storage
    )
    target_link_libraries(storage_engine PUBLIC oops::memory_engine Threads::Threads)
else()
    message(WARNING "Storage uses POSIX file APIs and is not built on native Windows. Use WSL/Linux for the integrated Storage target.")
endif()

if(MSVC)
    target_compile_options(memory_engine PRIVATE /W4)
    if(TARGET storage_engine)
        target_compile_options(storage_engine PRIVATE /W4)
    endif()
else()
    target_compile_options(memory_engine PRIVATE -Wall -Wextra -Wpedantic)
    if(TARGET storage_engine)
        target_compile_options(storage_engine PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

# -----------------------------
# Tests
# -----------------------------
if(BUILD_TESTING)
    add_executable(memory_store_test tests/memory/memory_store_test.cpp)
    target_link_libraries(memory_store_test PRIVATE oops::memory_engine Threads::Threads)
    add_test(NAME memory_store_test COMMAND memory_store_test)

    if(UNIX)
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
            add_executable(${test_name} tests/storage/${test_name}.cpp)
            target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/storage)
            target_link_libraries(${test_name} PRIVATE storage::engine)
            add_test(NAME ${test_name} COMMAND ${test_name})
        endforeach()

        add_executable(memory_engine_storage_integration_test
            tests/storage/memory_engine_storage_integration_test.cpp
        )
        target_include_directories(memory_engine_storage_integration_test
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/storage
        )
        target_link_libraries(memory_engine_storage_integration_test PRIVATE storage::engine)
        add_test(NAME memory_engine_storage_integration_test COMMAND memory_engine_storage_integration_test)
    endif()
endif()
