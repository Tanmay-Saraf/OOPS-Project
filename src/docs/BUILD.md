# Build and Test Guide

This guide covers building the networking server on Linux and running the project's tests.

## 1. Requirements

The current networking target uses Linux/POSIX APIs (TCP sockets, `epoll`, `eventfd`, `accept4`, and pthread signal handling), a C++20 compiler (GCC or Clang), CMake 3.20 or newer, GoogleTest, and POSIX threads.

The top-level CMake configuration uses `find_package(GTest REQUIRED)` and `find_package(Threads REQUIRED)`. GoogleTest is therefore required even when you only want to run the server.

### CachyOS / Arch Linux

~~~bash
sudo pacman -S --needed base-devel cmake gtest
~~~

### Ubuntu / Debian

~~~bash
sudo apt update
sudo apt install build-essential cmake libgtest-dev
~~~

Package names may vary by distribution and release.

## 2. Configure

Run from the repository root:

~~~bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
~~~

- `-S .` selects the current directory as the source tree.
- `-B build` puts generated build files in the `build/` directory.
- `CMAKE_BUILD_TYPE=Debug` enables a development-friendly debug build.

## 3. Compile

~~~bash
cmake --build build --parallel
~~~

To build only the server executable:

~~~bash
cmake --build build --target server_test --parallel
~~~

The executable is created at `build/server_test`.

## 4. Run tests

~~~bash
ctest --test-dir build --output-on-failure
~~~

The normal build registers the request-parser tests and response-serializer tests. When the optional security integration is configured, CMake also registers the security-adapter tests.

The two normal test executables can also be run directly:

~~~bash
./build/test_parses
./build/test_response_serializer
~~~

## 5. Start the server

The port argument is optional:

~~~bash
./build/server_test
~~~

The default port is `8080`. To choose another port:

~~~bash
./build/server_test 9090
~~~

The valid port range is 1 through 65535. Press `Ctrl+C` to request graceful shutdown.

## 6. Optional: build with Security_Defense

The security integration is optional. Without it, the server uses `AllowAllGuard`.

To enable it, pass the absolute path to a compatible `Security_Defense` checkout. That checkout must contain the `include/security` headers and the source files listed in the root `CMakeLists.txt`.

~~~bash
cmake -S . -B build-security \
  -DCMAKE_BUILD_TYPE=Debug \
  -DOOPS_SECURITY_DIR=/absolute/path/to/Security_Defense

cmake --build build-security --parallel
ctest --test-dir build-security --output-on-failure
~~~

Run the security-enabled executable with:

~~~bash
./build-security/server_test 8080
~~~

In this build, `server_main.cpp` constructs `SecurityGuardAdapter`. Its default log file is `security.log` in the server process's working directory.

Use a separate build directory such as `build-security` so the optional configuration does not overwrite the normal build's CMake cache.

## 7. Common build and run problems

| Problem | What to check |
|---|---|
| `Could NOT find GTest` | Install the GoogleTest development package, then configure CMake again. |
| CMake version is too old | Install CMake 3.20 or newer. |
| Address already in use / bind failure | Another process is listening on that port. Choose another port or stop the other process. |
| Invalid port | Pass one integer from 1 to 65535. |
| `nc: command not found` | Install a netcat package or use another TCP client. |
| Security headers or source files not found | Verify that `OOPS_SECURITY_DIR` points to the correct `Security_Defense` checkout. |

## 8. Clean rebuild

To regenerate the normal build directory from scratch:

~~~bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
~~~

Only remove the build directory. Do not remove source, test, or integration directories.
