# Networking C++ API Reference

This reference covers the public interfaces and important internal functions in the current networking implementation. Client protocol commands are documented in [USAGE.md](USAGE.md).

All networking classes and types below are in the `oops` namespace unless stated otherwise.

## 1. Request and response data types

### `Request`

Declared in `include/common/Request.hpp`.

~~~cpp
enum class Command {
    Set, Get, Delete, Exists, Keys, History, GetVersion,
    Ping, Quit, Save, Recover, Status, Invalid
};

struct Request {
    Command command = Command::Invalid;
    std::string key;
    std::string value;
    std::uint64_t version = 0;
};
~~~

A parsed request is represented by a `Command`, an optional key/value, and an optional version number.

### `Response`

Declared in `include/common/Response.hpp`.

~~~cpp
enum class ResponseStatus {
    Success, InvalidRequest, PermissionDenied, NotFound, InternalError
};

struct Response {
    ResponseStatus status = ResponseStatus::InternalError;
    std::string message;
    std::string value;
};
~~~

`status` selects the wire-protocol prefix. The serializer uses `value` when it is non-empty; otherwise it uses `message`.

## 2. Parser and serializer

### Parse a client command

Declared in `include/network/RequestParser.hpp`.

~~~cpp
Request parseRequest(const std::string& line);
~~~

- Input: one command line without its newline delimiter.
- Output: a populated `Request`.
- Invalid or malformed input produces a request whose command is `Command::Invalid`.

Examples:

~~~cpp
Request a = parseRequest("SET greeting hello world");
Request b = parseRequest("GET greeting");
Request c = parseRequest("GET_VERSION greeting 3");
~~~

### Serialize a response

Declared in `include/network/ResponseSerializer.hpp`.

~~~cpp
std::string serializeResponse(const Response& response);
~~~

Converts a `Response` to a newline-terminated protocol string such as `OK hello` or `ERROR NOT_FOUND Key not found`.

## 3. Server interface

Declared in `include/network/Server.hpp`.

### Constructor

~~~cpp
explicit Server(
    int port,
    std::unique_ptr<RequestGuard> guard = std::make_unique<AllowAllGuard>(),
    std::unique_ptr<MemoryEngineAdapter> memory = nullptr
);
~~~

- `port`: listening TCP port; valid range is 1–65535.
- `guard`: an owned request-guard implementation. The default is `AllowAllGuard`.
- `memory`: an owned memory-engine adapter. If omitted or `nullptr`, the server constructs `TemporaryMemoryEngine`.

The server takes ownership of the supplied `unique_ptr` objects.

### Public methods

~~~cpp
~Server();

void run();
void requestShutdown();

Server(const Server&) = delete;
Server& operator=(const Server&) = delete;
~~~

- `run()`: runs the `epoll` event loop. Call this on the server thread.
- `requestShutdown()`: requests shutdown and wakes the event loop through `eventfd`.
- Copying a `Server` is disabled because it owns file descriptors, worker threads, and connection state.

## 4. Request guard interface

Declared in `include/network/RequestGuard.hpp`.

~~~cpp
struct GuardDecision {
    bool allowed = true;
    std::string reason;
    std::string state;
};

class RequestGuard {
public:
    virtual ~RequestGuard() = default;
    virtual GuardDecision check(const Request& request) = 0;
};

class AllowAllGuard : public RequestGuard {
public:
    GuardDecision check(const Request&) override;
};
~~~

A guard decides whether a request may proceed. It can also provide a reason for denying a request and a state string for `STATUS`.

The default `AllowAllGuard` permits requests and leaves the state empty. The server therefore reports `UNKNOWN` for `STATUS` when using that default guard.

### Optional security adapter

Declared in `integration/SecurityGuardAdapter.hpp` when the security integration is available.

~~~cpp
explicit SecurityGuardAdapter(const std::string& logFile = "security.log");
GuardDecision check(const Request& request) override;
~~~

The adapter forwards requests to `SecurityEngine::processRequest()` and maps its decision and state into `GuardDecision`.

In the current dispatch path, `PING` and `QUIT` bypass the guard. Other valid commands, including `STATUS`, pass through `RequestGuard::check()`.

## 5. Memory-engine adapter

Declared in `include/network/MemoryEngineAdapter.hpp`.

~~~cpp
class MemoryEngineAdapter {
public:
    virtual ~MemoryEngineAdapter() = default;
    virtual Response execute(const Request& request) = 0;
};
~~~

This is the boundary between networking and storage. The server passes a parsed `Request` to the adapter and receives a `Response`. The adapter does not need to know about sockets, file descriptors, or `epoll`.

### Temporary implementation

Declared in `include/network/TemporaryMemoryEngine.hpp`.

~~~cpp
class TemporaryMemoryEngine final : public MemoryEngineAdapter {
public:
    Response execute(const Request& request) override;
};
~~~

The implementation protects its `unordered_map` with a mutex and currently supports `SET`, `GET`, `DELETE`, `EXISTS`, and `KEYS`. Other commands return `InvalidRequest` with the message `Unsupported memory command`.

## 6. Thread pool

Declared in `include/network/ThreadPool.hpp`.

~~~cpp
class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count);
    ~ThreadPool();

    void enqueue(std::function<void()> task);

private:
    void workerLoop();
};
~~~

The server constructs a pool with four workers. `enqueue()` submits work; worker threads repeatedly take and execute queued tasks. The destructor stops the task queue and joins the workers.

## 7. Thread-safe queue

Declared in `include/network/ThreadSafeQueue.hpp`.

~~~cpp
template <typename T>
class ThreadSafeQueue {
public:
    void push(T item);
    bool tryPop(T& item);
    bool waitPop(T& item);
    void stop();
};
~~~

- `push(item)`: adds an item and wakes a waiting consumer. Throws if the queue has been stopped.
- `tryPop(item)`: takes an item immediately if one is available; otherwise returns `false`.
- `waitPop(item)`: waits until an item is available or the queue is stopped. Returns `false` if stopped and empty.
- `stop()`: prevents further pushes and wakes all waiting consumers.

The same template is used for worker tasks and completed responses.

## 8. File-descriptor RAII wrapper

Declared in `include/network/UniqueFd.hpp`.

~~~cpp
class UniqueFd {
public:
    UniqueFd() noexcept = default;
    UniqueFd(int fd);
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;
    ~UniqueFd();

    UniqueFd(UniqueFd&& other) noexcept;
    UniqueFd& operator=(UniqueFd&& other) noexcept;

    int get() const noexcept;
};
~~~

`UniqueFd` owns one file descriptor and closes it in its destructor. It is movable but not copyable, preventing accidental duplicate ownership and double-close errors.

## 9. Important private server functions

These functions are implementation details of `Server`; normal client code does not call them directly.

| Function signature | Responsibility |
|---|---|
| `void setup()` | Creates the listening socket, configures it, creates `epoll` and `eventfd`, and registers the relevant descriptors. |
| `void acceptClients()` | Accepts pending clients and registers their non-blocking sockets with `epoll`. |
| `void handleClientEvent(const int fd)` | Reads available bytes, splits newline-delimited commands, enforces the request-size limit, and parses requests. |
| `void handleRequest(const Request& request, int client_fd)` | Queues a request in that client's state and starts dispatch when possible. |
| `void dispatchNextRequest(std::uint64_t client_id)` | Dispatches the next queued request for a client to the worker pool. |
| `void handleWriteEvent(int fd)` | Sends queued response bytes and manages write-side connection state. |
| `void updateClientEvents(int fd, std::uint32_t flags)` | Updates the events registered for a client descriptor in `epoll`. |
| `void publishResponse(CompletedResponse completed)` | Pushes a worker result to the completion queue and wakes the event loop through `eventfd`. |
| `bool checkAllClientBuffersEmpty() const` | Checks whether any client still has response bytes waiting to be sent. |

The key ordering rule is that a client has at most one request being processed at a time. Further requests from that same client wait in its pending queue. Different clients can be processed by different worker threads.

## 10. Executable entry point

Declared in `src/network/server_main.cpp`.

~~~cpp
int main(int argc, char* argv[]);
~~~

The entry point parses the optional port, constructs the server (with `SecurityGuardAdapter` when the optional security build is enabled), starts the event loop on a thread, and handles `SIGINT`/`SIGTERM` to request graceful shutdown.

The local helper is:

~~~cpp
int parsePort(const char* text);
~~~

It validates that the argument is a complete integer in the range 1–65535.
