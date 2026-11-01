# OOPS Project — Networking Server

This repository contains the networking layer for the OOPS project. The networking implementation is a Linux TCP server written in C++20 using raw POSIX sockets and `epoll` (not Boost.Asio or standalone Asio).

## What the networking layer does

- Accepts multiple TCP clients using a non-blocking listening socket.
- Uses `epoll` to monitor socket readiness without dedicating one thread to every connection.
- Parses newline-delimited text requests.
- Uses a four-worker thread pool to execute requests away from the event loop.
- Maintains a per-client request queue so requests from the same client are processed in order.
- Uses `eventfd` to wake the event loop when workers publish completed responses.
- Passes storage requests through a `MemoryEngineAdapter` interface.
- Passes eligible requests through a `RequestGuard` interface; an optional Security_Defense adapter can be enabled at build time.
- Uses RAII (`UniqueFd`) to manage file descriptors.

## Current implementation scope

The default backend is `TemporaryMemoryEngine`, which stores data in an in-memory `unordered_map`. It currently implements `SET`, `GET`, `DELETE`, `EXISTS`, and `KEYS`. Data is lost when the server process exits.

`HISTORY` and `GET_VERSION` are recognised by the parser and routed to the memory adapter, but the temporary backend does not implement them. `SAVE` and `RECOVER` are parsed but are not implemented in the server dispatch path. The broader project vision includes versioned storage, persistence, and recovery; do not confuse that intended scope with the current capabilities of this networking branch.

## Quick start

Requirements: Linux, C++20 compiler, CMake 3.20+, GoogleTest, and POSIX threads.

~~~bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/server_test 8080
~~~

In another terminal, connect with netcat:

~~~bash
nc 127.0.0.1 8080
~~~

Try:

~~~text
PING
SET greeting hello world
GET greeting
EXISTS greeting
KEYS
QUIT
~~~

Press `Ctrl+C` in the server terminal for graceful process shutdown. `QUIT` closes only the client connection.

## Documentation

- [Build and test guide](docs/BUILD.md) — dependencies, CMake commands, tests, optional security build, troubleshooting.
- [Server usage](docs/USAGE.md) — protocol syntax, command support, examples, response format.
- [C++ API reference](docs/API.md) — public interfaces, constructors, methods, and important internal functions.
- [Manual presentation runbook](docs/PRESENTATION.md) — exact terminal sequence, expected output, and explanation of the request lifecycle.

## Optional Security_Defense integration

The normal build uses `AllowAllGuard`. To enable the adapter, configure CMake with `-DOOPS_SECURITY_DIR=/absolute/path/to/Security_Defense`. See [docs/BUILD.md](docs/BUILD.md) for the expected checkout layout and commands.

## Important operational notes

- The server currently uses a plaintext TCP protocol; it does not provide TLS or client authentication.
- It binds to `INADDR_ANY`, meaning it listens on the machine's available network interfaces. Use it only on a trusted development network unless additional protections are added.
- The default server port is `8080`; a custom port can be supplied as the first command-line argument.
- The maximum request line size is 4096 bytes.
