# Server Usage and Command Reference

This document explains how to connect to the TCP server and which commands are currently usable.

## 1. Start the server

From the repository root:

~~~bash
./build/server_test 8080
~~~

The port is optional; the default is `8080`.

The server binds its listening socket to `INADDR_ANY`, so it listens on the machine's available network interfaces. For a local demonstration, connect through `127.0.0.1`. Do not expose this development server to an untrusted network: this networking layer does not provide TLS or client authentication.

## 2. Connect with netcat

Open a second terminal:

~~~bash
nc 127.0.0.1 8080
~~~

Type one command per line. Each request must end with a newline. The server sends one newline-terminated response for each processed request.

Type `QUIT` to close that client connection. It does **not** stop the server process.

## 3. Command syntax

Commands are uppercase. Keys are single whitespace-delimited tokens. `SET` is the exception for its value: everything after the key is treated as the value, so values may contain spaces.

| Command | Syntax | Meaning |
|---|---|---|
| Set a value | `SET <key> <value...>` | Store or replace a key's value in the temporary memory backend. |
| Read a value | `GET <key>` | Return the current value for the key. |
| Delete a key | `DELETE <key>` | Remove the key from the temporary memory backend. |
| Check a key | `EXISTS <key>` | Return `true` or `false`. |
| List keys | `KEYS` | Return the keys currently in the temporary memory backend. |
| Check server/security state | `STATUS` | Return the state reported by the configured request guard. |
| Ping | `PING` | Check that the server responds. |
| Close this client | `QUIT` | Send `BYE` and close this client connection. |
| Read history | `HISTORY <key>` | Parsed and routed to the memory adapter, but unsupported by the current temporary backend. |
| Read a version | `GET_VERSION <key> <version>` | Parsed and routed to the memory adapter, but unsupported by the current temporary backend. Version must be a positive unsigned 64-bit integer. |
| Save | `SAVE` | Parsed, but not implemented in the server dispatch path. |
| Recover | `RECOVER` | Parsed, but not implemented in the server dispatch path. |

### Current implementation status

It is important to distinguish parsing from execution:

- **Implemented by the temporary backend:** `SET`, `GET`, `DELETE`, `EXISTS`, `KEYS`.
- **Handled directly by the server:** `PING`, `QUIT`, `STATUS`.
- **Parsed and routed, but unsupported by the temporary backend:** `HISTORY`, `GET_VERSION`. They currently return `ERROR INVALID_REQUEST Unsupported memory command`.
- **Parsed, but not implemented in server dispatch:** `SAVE`, `RECOVER`. They currently return `ERROR INTERNAL_ERROR Request processing not implemented`.

The temporary backend stores data only in RAM. All records are lost when the server process exits or restarts. It is a stand-in behind `MemoryEngineAdapter`, not the persistent/versioned storage engine.

## 4. Recommended manual demo

With the server running in Terminal 1, connect from Terminal 2:

~~~bash
nc 127.0.0.1 8080
~~~

Enter these commands one at a time:

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

With the default `AllowAllGuard` and temporary memory backend, the responses from a fresh server process are:

~~~text
OK PONG
OK UNKNOWN
OK Value stored
OK hello world
OK true
OK greeting
OK Key deleted
ERROR NOT_FOUND Key not found
OK BYE
~~~

`STATUS` returns `UNKNOWN` with the default `AllowAllGuard` because that guard does not report a security state. With `SecurityGuardAdapter`, the response can report a state such as `RUNNING`, `SUSPECTED`, or `LOCKDOWN`, depending on the security engine.

The order of keys returned by `KEYS` is not guaranteed because the temporary backend uses an `unordered_map`.

## 5. Multi-client check

The temporary memory engine is owned by the server, not by an individual client. To demonstrate that clients use the same in-memory store:

1. Keep the server running.
2. In Terminal 2, connect with `nc 127.0.0.1 8080` and send `SET shared hello`. Expected: `OK Value stored`.
3. In Terminal 3, connect to the same port and send `GET shared`. Expected: `OK hello`.

This demonstrates shared server-side state across separate TCP connections.

## 6. Request and response format

### Requests

- One command per line.
- A newline (`\n`) terminates a request. A trailing carriage return is also handled, so CRLF line endings are accepted.
- The maximum request line size is 4096 bytes. Oversized input is rejected as an invalid request.
- `SET` keeps the full remainder of the line as its value; other commands reject unexpected extra arguments.

Examples:

~~~text
SET title Distributed Systems
GET title
GET title extra
GET_VERSION title 2
GET_VERSION title 0
~~~

The third and fifth examples are invalid.

### Responses

Every response ends in a newline. The first token indicates the result:

| Response prefix | Meaning |
|---|---|
| `OK` | Request succeeded. |
| `ERROR INVALID_REQUEST` | Request syntax is invalid, or the backend rejected the command as unsupported. |
| `ERROR PERMISSION_DENIED` | The request guard denied the operation. |
| `ERROR NOT_FOUND` | The requested key does not exist. |
| `ERROR INTERNAL_ERROR` | The server could not process the request or the operation is not implemented. |

If a response has a non-empty `value`, the serializer sends that value after the status prefix. Otherwise, it sends the response `message`.

Examples:

~~~text
OK apple
OK Value stored
ERROR NOT_FOUND Key not found
ERROR INVALID_REQUEST Invalid request
~~~

## 7. Quick one-shot test

Send a batch of commands without opening an interactive prompt:

~~~bash
printf 'SET fruit apple\nGET fruit\nEXISTS fruit\nQUIT\n' | nc 127.0.0.1 8080
~~~

Expected response:

~~~text
OK Value stored
OK apple
OK true
OK BYE
~~~

## 8. Stop the server

Return to Terminal 1 and press `Ctrl+C`. The main program receives the shutdown signal, asks the server event loop to stop, and joins the server thread.

`QUIT` is a client-level command; `Ctrl+C` is the process-level shutdown action.
