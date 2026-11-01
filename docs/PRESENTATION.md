# Manual Presentation Runbook

This is a repeatable sequence for demonstrating the networking server without relying on an automated demo script.

## Before the presentation

From the repository root, build and run the tests once:

~~~bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
~~~

For the presentation, keep two terminal windows visible. A third terminal is optional for the multi-client demonstration.

## Demo 1: start the server

In **Terminal 1**:

~~~bash
./build/server_test 8080
~~~

Expected startup message:

~~~text
Server listening on port 8080. Press Ctrl+C to stop.
~~~

Explain:

- The server creates a TCP listening socket.
- It uses non-blocking I/O and registers descriptors with `epoll`.
- The default port is `8080`; the executable also accepts a custom port.

## Demo 2: connect a client

In **Terminal 2**:

~~~bash
nc 127.0.0.1 8080
~~~

Explain that this is a raw TCP text protocol, not HTTP. Each command is one newline-terminated request.

## Demo 3: run the command sequence

Type these lines into Terminal 2, one at a time:

~~~text
PING
STATUS
SET greeting hello world
GET greeting
EXISTS greeting
KEYS
DELETE greeting
GET greeting
QUIT
~~~

With the default `AllowAllGuard`, the temporary memory backend, and a fresh server process, expect:

| Input | Expected response | What it demonstrates |
|---|---|---|
| `PING` | `OK PONG` | Server-level request and response. |
| `STATUS` | `OK UNKNOWN` | State reported by the default allow-all guard. |
| `SET greeting hello world` | `OK Value stored` | Stores a value containing spaces. |
| `GET greeting` | `OK hello world` | Reads the stored value. |
| `EXISTS greeting` | `OK true` | Checks whether the key exists. |
| `KEYS` | `OK greeting` | Lists current keys. |
| `DELETE greeting` | `OK Key deleted` | Deletes the key. |
| `GET greeting` | `ERROR NOT_FOUND Key not found` | Correctly handles a missing key. |
| `QUIT` | `OK BYE` | Closes this client connection. |

`QUIT` closes only Terminal 2's client connection. The server continues listening.

## Demo 4: prove that state is shared between clients (optional)

Do this before sending `QUIT` in Demo 3, or open fresh client terminals afterwards.

1. In Terminal 2, connect with `nc 127.0.0.1 8080` and send:

   ~~~text
   SET shared from-client-one
   ~~~

   Expected: `OK Value stored`.

2. In **Terminal 3**, connect separately:

   ~~~bash
   nc 127.0.0.1 8080
   ~~~

3. Send:

   ~~~text
   GET shared
   ~~~

   Expected: `OK from-client-one`.

This demonstrates that separate clients access the same server-owned memory backend.

## Demo 5: show graceful shutdown

Return to Terminal 1 and press **Ctrl+C**.

The main program receives the signal and calls `Server::requestShutdown()`. That method wakes the event loop through `eventfd`, allowing the server thread to finish and be joined.

Expected final message:

~~~text
Server stopped cleanly.
~~~

## What to explain about the internal flow

Use this sequence while pointing at the implementation:

1. **Accept:** `acceptClients()` accepts a client on the non-blocking listening socket.
2. **Wait for readiness:** `epoll` reports when the listening socket or a client socket is ready.
3. **Read and parse:** `handleClientEvent()` reads bytes, splits requests at newline characters, and calls `parseRequest()`.
4. **Queue per client:** `handleRequest()` stores the parsed request in that client's pending queue.
5. **Dispatch:** `dispatchNextRequest()` sends the next request to the four-worker thread pool. It allows only one in-flight request per client, preserving that client's request order.
6. **Guard and execute:** the worker checks the request guard (except for `PING` and `QUIT`), then handles the command or calls `MemoryEngineAdapter::execute()`.
7. **Publish result:** the worker creates a `CompletedResponse`, pushes it onto the completed-response queue, and signals `eventfd`.
8. **Send response:** the event loop drains completed responses, uses `serializeResponse()`, and enables write readiness for the client socket.
9. **Shutdown:** `requestShutdown()` wakes the event loop so it can finish without waiting indefinitely in `epoll_wait()`.

## Useful implementation points to mention

- **Why `epoll`?** It lets one event loop monitor many file descriptors without dedicating one thread to every client connection.
- **Why non-blocking sockets?** A read or write does not hold the event loop waiting for one client.
- **Why a worker pool?** Request execution is moved away from the event loop, so the loop can continue handling socket readiness.
- **Why a per-client queue?** Work from one client is processed in order even though execution uses multiple worker threads.
- **Why `eventfd`?** Worker threads need a safe way to wake the thread blocked in `epoll_wait()` when a response is ready.
- **Why `UniqueFd`?** It gives file descriptors RAII ownership and closes them automatically.
- **Why `MemoryEngineAdapter`?** It decouples networking from the storage implementation. The server depends on the adapter interface rather than a concrete memory engine.

## Be precise about current limitations

Do not claim these features are implemented by the current temporary backend:

- The temporary memory engine is RAM-only; data disappears when the server exits.
- `HISTORY` and `GET_VERSION` are parsed and routed to the adapter, but the temporary backend does not implement them.
- `SAVE` and `RECOVER` are parsed, but are not handled by the server's dispatch switch.
- The default `AllowAllGuard` permits requests and reports no state, so `STATUS` returns `UNKNOWN`.
- The TCP server does not add TLS or client authentication.

With the optional `Security_Defense` integration configured, the server uses `SecurityGuardAdapter` instead of `AllowAllGuard`. The reported status then comes from the security engine and requests may be denied by the guard.

## Quick troubleshooting during the demo

| Symptom | Fix |
|---|---|
| Server cannot bind to port 8080 | Start it on another port, e.g. `./build/server_test 9090`, and connect to that port. |
| `nc` is missing | Install netcat or use another TCP client. |
| `STATUS` says `UNKNOWN` | This is expected with the default `AllowAllGuard`. |
| `GET` returns `ERROR NOT_FOUND` | The key has not been set, or it was already deleted. |
| Data disappears after restart | Expected: the current backend is temporary RAM storage. |
| `QUIT` stops responding to that terminal | Expected: `QUIT` closes that client, not the server. |
