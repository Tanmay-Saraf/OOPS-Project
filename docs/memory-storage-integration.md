# Memory Engine + Storage Integration

This project now builds the Memory Engine and the supplied POSIX WAL/Recovery module together.

## What is connected

- `MemoryEngine::execute(SET/DELETE)` calls its registered `WalAppendHandler` through `MemoryStore`.
- `storage::make_wal_append_handler(writer)` converts the exact `Version` to a `WalRecord` and synchronously appends/syncs it.
- `storage::make_memory_replay_handler(engine)` converts WAL records back to `Version` objects and calls the public `MemoryEngine::replayVersion()` API.
- `storage::make_memory_clear_handler(engine)` forwards the RecoveryManager clear request to `MemoryEngine::clear()`.
- `RecoveryManager` supplies startup replay and permanent point-in-time rollback.

## Startup wiring

Create the writer before the engine so the engine (and its callback) is destroyed before the writer:

```cpp
storage::WalWriter writer("data/kittydb.wal");
oops::MemoryEngine engine;
storage::RecoveryManager recovery;

auto report = recovery.recover(
    "data/kittydb.wal",
    storage::make_memory_replay_handler(engine)
);

if(!report.fully_recovered()) {
    throw std::runtime_error("KittyDB WAL recovery did not complete successfully");
}

engine.setWalAppendHandler(storage::make_wal_append_handler(writer));

// Only after successful recovery and callback registration should the server
// start accepting client requests.
```

`WalWriter` creates the WAL file if it does not exist. Recovery should run before the server accepts requests. Do not start serving if `fully_recovered()` is false; inspect `status`, `error_offset`, and `replay_failed` and decide how the operator should handle the damaged log.

The callback is synchronous. A successful return means `WalWriter::append()` completed its write and `fdatasync()`. If the write/sync fails, the writer throws; the MemoryStore does not append the version to RAM and `MemoryEngine::execute()` returns `InternalError`.

The callback captures `WalWriter` by reference. Keep the writer alive for the entire time the engine can execute writes. The declaration order in the example achieves the correct destruction order.

## Permanent point-in-time rollback

The current `RecoveryManager::rollback_to()` locks the WAL writer while it replaces the WAL file, clears memory, and replays retained records. **Quiesce the server first:** stop accepting new requests and wait for in-flight SET/DELETE operations to finish before calling rollback. Resume serving only if `fully_recovered()` is true.

```cpp
auto report = recovery.rollback_to(
    writer,
    storage::make_memory_clear_handler(engine),
    storage::make_memory_replay_handler(engine),
    target_timestamp_us
);

if(!report.fully_recovered()) {
    throw std::runtime_error("KittyDB rollback did not complete successfully");
}
```

The target timestamp is Unix epoch microseconds, matching `WalRecord::timestamp`.

## Build requirements

The supplied WAL writer uses POSIX APIs (`open`, `write`, `fdatasync`, `mkstemp`, `rename`, and `fsync`). Build the integrated Storage target under Linux or WSL:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
ctest --test-dir build --output-on-failure --timeout 30
```

Native Windows builds continue to build/run the Memory Engine tests; CMake skips the POSIX Storage target there.

## Important concurrency boundary

The write callback must only append to the WAL. It must not call back into MemoryEngine/MemoryStore or perform another database write. The MemoryStore write-serialization mutex is held for the full version-generation -> callback -> memory-commit sequence.

Rollback is a maintenance operation and must be run while request processing is paused and in-flight writes have drained. The RecoveryManager currently holds the WAL writer mutex while calling the clear/replay handlers, so allowing active writes to contend during rollback can create a lock-order deadlock.
