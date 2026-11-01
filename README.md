# Ransomware-Resilient Database System

This repository contains the Security & Defense component of a broader
ransomware-resilient database project. The security component can evaluate
parsed requests and return an approval/denial decision; the full Network Guard,
Memory Engine, and Disk Storage & Recovery integration must be validated
separately before the project can claim end-to-end database recovery behavior.

## Security & Defense capabilities

- Canary-record detection for configured canary keys.
- Sliding-window modification-rate, per-minute volume, and distinct-key signals.
- Shannon entropy analysis for sufficiently long `SET` values.
- Monotonic `RUNNING` → `SUSPECTED` → `LOCKDOWN` state escalation.
- In-process `oops::SecurityEngine::processRequest(const oops::Request&)` API.
- Caller-safe decision reasons and a thread-protected internal security log.
- `LOCKDOWN` permits only `GET` through the security engine.
- `SAVE`, `RECOVER`, `PING`, `QUIT`, invalid, and unknown commands are not
  approved for forwarding by this engine. Network Guard owns protocol-only
  commands and must handle `PING`/`QUIT` itself.

## Build and test

Requires CMake 3.16+ and a C++20 compiler.

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The tests use always-active `TEST_CHECK` assertions, so checks remain enabled
in Release builds as well as Debug builds.

## Public API example

Build and run the example with:

```powershell
cmake --build build --target SecurityEngineExample
.\build\SecurityEngineExample.exe
```

The example shows an ordinary `GET`, a canary-triggered lockdown, a permitted
`GET` during lockdown, and a denied write during lockdown. Security approval
is not database execution: an allowed request may still fail in the Memory
Engine.

## Installable CMake target

You can install the library and its public headers with:

```powershell
cmake --install build --prefix install
```

The exported target is `oops::security_engine`. The install contains the stable
public headers only; detector and state-manager implementation headers remain
internal to this component.

## Security limitations

No trusted client authentication/authorization or privileged recovery/unlock
mechanism is implemented. Detector thresholds are initial policy values, not
empirically calibrated production thresholds. This component by itself does
not guarantee append-only persistence, rollback, or data recovery. Those
claims require the completed and tested storage and recovery modules.
