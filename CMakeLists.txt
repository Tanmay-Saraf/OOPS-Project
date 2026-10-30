cmake_minimum_required(VERSION 3.20)

project(KittyDBMemoryEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
find_package(Threads REQUIRED)

# Memory Engine implementation files
set(MEMORY_ENGINE_SOURCES
    src/memory/VersionChain.cpp
    src/memory/MemoryStore.cpp
    src/memory/MemoryEngine.cpp
)

# Version's implementation may remain inline in Version.hpp.
# Include Version.cpp only if the file exists in this project.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/memory/Version.cpp")
    list(APPEND MEMORY_ENGINE_SOURCES src/memory/Version.cpp)
endif()

# Build the Memory Engine as a reusable library
add_library(memory_engine ${MEMORY_ENGINE_SOURCES})
add_library(oops::memory_engine ALIAS memory_engine)

target_include_directories(memory_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(memory_engine
    PUBLIC
        Threads::Threads
)

if(MSVC)
    target_compile_options(memory_engine PRIVATE /W4)
else()
    target_compile_options(memory_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Standalone Memory Engine tests (the test file uses assert, not GoogleTest)
if(BUILD_TESTING)
    add_executable(memory_store_test
        tests/memory/memory_store_test.cpp
    )

    target_link_libraries(memory_store_test
        PRIVATE
            oops::memory_engine
            Threads::Threads
    )

    if(MSVC)
        target_compile_options(memory_store_test PRIVATE /W4)
    else()
        target_compile_options(memory_store_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME memory_store_test COMMAND memory_store_test)
endif()