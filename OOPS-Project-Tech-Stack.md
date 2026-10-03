# Crash-Resilient / Ransomware-Resistant Key-Value Database

## Recommended Technology Stack

The project is recommended to use **C++20** as the core language, with supporting tools and libraries for networking, testing, logging, and build management.

### Core Stack

| Component | Technology | Purpose |
|---|---|---|
| Core language | **C++20** | Main implementation language; OOP, memory/data-structure control, concurrency, file I/O |
| Networking | **Standalone Asio** | TCP client/server networking and asynchronous connections |
| Build system | **CMake** | Portable project configuration and compilation |
| Testing | **GoogleTest** | Unit and integration testing |
| Application logging | **spdlog** | Debug, informational, warning, and security-event logs |
| Optional serialization/configuration | **nlohmann/json** | JSON for configuration, debugging, or API responses; not for the core database storage format |
| Version control | **Git + GitHub** | Collaboration, branches, pull requests, and project history |
| CI | **GitHub Actions** | Optional automated build and test execution |

---

# System Architecture

The proposed system consists of four major components:

```text
                    ┌──────────────────────┐
Client ────────────►│     Network Guard    │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │ Security & Defense   │
                    │       Engine         │
                    └──────────┬───────────┘
                               │
                     ┌─────────┴─────────┐
                     │                   │
                  SAFE                 ATTACK
                     │                   │
                     ▼                   ▼
              ┌────────────┐      ┌──────────┐
              │   Memory   │      │ Lockdown │
              │   Engine   │      └──────────┘
              └─────┬──────┘
                    │
                    ▼
              ┌────────────┐
              │    Disk    │
              │  Storage   │
              └────────────┘
```

## 1. Network Guard

Responsible for:

- TCP client/server communication
- Accepting multiple client connections
- Request parsing
- Request validation
- Sending responses
- Passing valid requests to the internal database/security layers
- Concurrency handling

Recommended technology:

**C++20 + Standalone Asio**

---

# 2. Security & Defense Engine

This component detects suspicious activity and can put the database into lockdown.

## 2.1 Change-Rate Detection

Track activity such as:

```text
updates / second
updates / minute
unique keys modified / minute
```

Example:

```text
Normal:
5 updates/minute
8 updates/minute
6 updates/minute

Sudden:
15,000 updates/minute
```

A large deviation from normal activity can trigger a security response.

## 2.2 Entropy / Randomization Detection

Calculate the Shannon entropy of values.

Normal-looking values:

```text
Tanmay
CSE
NIT Warangal
```

Potentially suspicious values:

```text
f8A#2kL91@x...
```

The system can use entropy as one signal when deciding whether activity resembles mass encryption or randomized data modification.

## 2.3 Canary Records

Maintain protected records that should never normally be modified.

Example:

```text
__CANARY_001__ = DO_NOT_MODIFY
```

If a canary is modified:

```text
CANARY VIOLATION
       ↓
LOCKDOWN
```

## 2.4 Combined Security Decision

The security engine can combine several signals:

```text
Change-rate score
       +
Entropy score
       +
Canary violation
       ↓
Risk assessment
       ↓
LOCKDOWN
```

The exact thresholds and scoring model should be designed and documented by the team.

---

# 3. Lockdown State Machine

A simple state model:

```text
RUNNING
   │
   │ suspicious activity
   ▼
SUSPECTED
   │
   │ threshold reached
   ▼
LOCKDOWN
   │
   │ administrator recovery
   ▼
RECOVERY
   │
   ▼
RUNNING
```

When the system enters lockdown:

- Stop accepting new database modifications.
- Keep the system available for appropriate read/recovery operations.
- Preserve the historical data required for recovery.
- Allow an administrator-controlled recovery procedure.

The exact transition rules should be defined during architecture design.

---

# 4. Memory Engine

The Memory Engine provides fast access to active data.

A possible starting design is:

```text
unordered_map<Key, VersionChain>
```

For example:

```text
Key: user:101

VersionChain:
 ├── v1 → Tanmay
 ├── v2 → Rahul
 ├── v3 → Tanmay
 └── v4 → encrypted-looking value
```

The Memory Engine should eventually support operations such as:

```text
PUT
GET
DELETE
HISTORY
```

and historical lookups.

### Important

The original project proposal mentioned a Binary Search Tree, but the updated requirements do not necessarily require a BST.

Therefore, the team should choose the in-memory data structure based on the final access patterns rather than committing to a BST prematurely.

---

# 5. Append-Only Disk Storage

The database should preserve historical changes instead of simply overwriting the previous value.

Example:

```text
Version 1:
email = old@gmail.com

Version 2:
email = new@gmail.com
```

The old version remains available.

Conceptually, the storage layer can maintain a sequence of records:

```text
1. PUT A 10
2. PUT B 20
3. PUT A 15
4. DELETE B
```

This history can be replayed to reconstruct database state.

## Suggested Custom Record Format

A possible binary record layout:

```text
+----------+------+-------+-----------+----------+
| Type     | Key  | Value | Timestamp | Checksum |
+----------+------+-------+-----------+----------+
```

The exact binary format should be designed by the team.

The core database history should be implemented by the team rather than delegated to an existing database.

Recommended standard C++ facilities include:

```text
std::fstream
std::ifstream
std::ofstream
std::filesystem
```

---

# 6. Recovery

Recovery has two related goals.

## 6.1 Crash Recovery

After a restart, replay the persistent history to reconstruct the current database state.

Conceptually:

```text
Disk log
   ↓
Read records
   ↓
Replay operations
   ↓
Rebuild memory state
```

## 6.2 Point-in-Time / Historical Recovery

Because old versions are retained, the system can reconstruct an earlier state.

Example:

```text
10:00   A = 10
10:01   B = 20
10:02   A = 15
10:03   A = XYZ
```

Request:

```text
recover(timestamp = 10:01)
```

Result:

```text
A = 10
B = 20
```

The exact versioning and timestamp model needs to be designed before implementation.

---

# 7. Why Append-Only Storage Matters

A conventional overwrite-based database might do:

```text
A = 10
     ↓
A = 20
     ↓
A = XYZ
```

and only retain:

```text
A = XYZ
```

An append-only system instead preserves:

```text
A = 10
A = 20
A = XYZ
```

This historical information is what makes point-in-time reconstruction possible.

---

# 8. Networking and Request Model

A simple command protocol could eventually support:

```text
PUT key value
GET key
DELETE key
HISTORY key timestamp
RECOVER timestamp
```

Example:

```text
Client:
PUT user1 Tanmay

        ↓

Network Guard
        ↓
Security Engine
        ↓
Memory Engine
        ↓
Disk Storage
```

The final command syntax should be agreed upon by the entire team before separate modules are implemented.

---

# 9. Serialization

For the core database storage, prefer a custom binary representation rather than storing every record as JSON.

JSON can still be useful for:

- Configuration
- Debugging
- Administrative APIs
- Human-readable responses

Recommended optional library:

**nlohmann/json**

The database's persistent record format should remain a custom design.

---

# 10. Logging

Use **spdlog** for application and diagnostic logs.

Example:

```text
[INFO] Client connected
[INFO] PUT key=user:101
[WARN] High write rate detected
[WARN] Entropy threshold exceeded
[CRITICAL] CANARY VIOLATION
[CRITICAL] SYSTEM LOCKDOWN
```

Important distinction:

```text
Application logs
        ↓
     spdlog

Database history / persistent records
        ↓
Custom storage implementation
```

These are two different systems.

---

# 11. Testing

Use **GoogleTest**.

Recommended test categories:

```text
tests/
├── network/
├── security/
├── memory/
├── storage/
└── recovery/
```

Important tests include:

### Storage

```text
Write record
Read record
Append multiple records
Detect corrupted/incomplete records
```

### Recovery

```text
Replay log
Recover after simulated crash
Reconstruct current state
Recover historical state
```

### Security

```text
Normal write rate
High write rate
High-entropy values
Canary modification
Lockdown activation
```

### Memory

```text
PUT
GET
DELETE
Multiple versions
Historical lookup
```

### Network

```text
Client connection
Request parsing
Invalid commands
Multiple clients
Concurrent requests
```

---

# 12. CMake

Use CMake as the project build system.

A typical build workflow should eventually be:

```bash
cmake -S . -B build
cmake --build build
```

Tests can then be integrated into the CMake configuration.

Do not commit the generated `build/` directory to Git.

---

# 13. Recommended Repository Structure

For now:

```text
OOPS-Project/
├── README.md
├── docs/
├── src/
└── tests/
```

Once the C++ architecture is finalized, we can expand it to something like:

```text
OOPS-Project/
├── CMakeLists.txt
├── README.md
│
├── docs/
│   ├── architecture.md
│   ├── design.md
│   ├── protocol.md
│   └── storage-format.md
│
├── include/
│   ├── network/
│   ├── security/
│   ├── memory/
│   ├── storage/
│   └── recovery/
│
├── src/
│   ├── network/
│   ├── security/
│   ├── memory/
│   ├── storage/
│   └── recovery/
│
└── tests/
    ├── network/
    ├── security/
    ├── memory/
    ├── storage/
    └── recovery/
```

Do not create all of these directories until the architecture is agreed upon.

---

# 14. Suggested Team Division

For a four-person team:

| Member | Main Ownership | Responsibilities |
|---|---|---|
| **Member 1** | Network Guard | TCP server, clients, connections, parsing, concurrency |
| **Member 2** | Security & Defense | Rate detector, entropy detector, canary, security state machine, lockdown |
| **Member 3** | Memory Engine | In-memory data structure, version chains, PUT/GET/DELETE, historical lookup |
| **Member 4** | Disk Storage & Recovery | Append-only storage, record format, persistence, replay, crash recovery, point-in-time recovery |

The components should not be developed in isolation. All four members need to agree on the shared interfaces and data/command formats first.

---

# 15. Shared Architecture Decisions Before Coding

Before splitting into implementation branches, the team should agree on:

1. What exactly constitutes a database record?
2. What commands does the client support?
3. What does `PUT` mean in an append-only database?
4. How is `DELETE` represented?
5. How are versions identified?
6. How are timestamps represented?
7. How is historical state reconstructed?
8. What constitutes suspicious activity?
9. What thresholds trigger lockdown?
10. What happens after lockdown?
11. How does administrator recovery work?
12. What happens when a persistent record is partially written?
13. How are concurrent clients handled?
14. What is the exact on-disk record format?
15. What interfaces connect the four major modules?

These decisions should be documented in `docs/` before the team begins implementing the individual components.

---

# 16. Development Strategy

Do not immediately implement all four components independently.

A better progression is:

```text
Phase 1
Requirements + architecture
        ↓
Phase 2
Command protocol + data model
        ↓
Phase 3
Basic in-memory database
        ↓
Phase 4
Append-only persistence
        ↓
Phase 5
Crash recovery
        ↓
Phase 6
Version / historical recovery
        ↓
Phase 7
Security tripwires
        ↓
Phase 8
Lockdown
        ↓
Phase 9
Networking + concurrency
        ↓
Phase 10
Integration + testing
```

This minimizes the risk of four components being implemented with incompatible assumptions.

---

# 17. Final Recommended Stack

```text
Language:
C++20

Networking:
Standalone Asio

Build:
CMake

Testing:
GoogleTest

Application logging:
spdlog

Optional JSON:
nlohmann/json

Persistence:
Custom append-only storage using C++ file I/O

In-memory storage:
Custom version-aware structure, initially based around
std::unordered_map + version chains

Version control:
Git + GitHub

CI:
GitHub Actions
```

## Core principle

Use libraries for **infrastructure**:

```text
Networking → Asio
Testing    → GoogleTest
Logging    → spdlog
Build      → CMake
```

but implement the **interesting database/security mechanisms yourselves**:

```text
Versioning
Append-only storage
Record format
Replay
Recovery
Historical reconstruction
Change-rate detection
Entropy detection
Canary
Lockdown
```

That keeps the project technically substantial and ensures the important parts of the system are actually your team's work.
