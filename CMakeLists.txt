cmake_minimum_required(VERSION 3.20)

project(KittyDBMemoryEngineTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
find_package(Threads REQUIRED)

add_executable(memory_store_test
    tests/memory/memory_store_test.cpp
    src/memory/Version.cpp
    src/memory/VersionChain.cpp
    src/memory/MemoryStore.cpp
)

target_include_directories(memory_store_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(memory_store_test
    PRIVATE
        Threads::Threads
)

if(MSVC)
    target_compile_options(memory_store_test PRIVATE /W4)
else()
    target_compile_options(memory_store_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_test(NAME memory_store_test COMMAND memory_store_test)
endif()