# 🛡️ Ransomware-Resilient Database: Team Role Divisions & AI Prompts

This document outlines the four specialized roles for building the custom C++20 database. Each section includes a detailed responsibility breakdown and a tailored prompt that each developer can paste directly into their AI assistant to immediately start generating relevant, context-aware code.

---

## Member 1: Network Guard (TCP Server & Concurrency)
**Goal:** Build a high-performance, multithreaded TCP server entirely from scratch using raw Linux POSIX sockets[cite: 1]. You are the system's frontline, managing concurrent client connections and passing valid data deeper into the database.

**Core Responsibilities:**
*   Manage raw TCP client/server communication without external networking frameworks (no Asio)[cite: 1].
*   Implement an asynchronous `epoll` event loop to monitor hundreds of file descriptors simultaneously.
*   Build a custom thread pool to handle concurrent request parsing safely.
*   Prevent race conditions and deadlocks using `std::mutex` and `std::condition_variable`.

**AI Prompt / Starting Context (Copy & Paste):**
> "Act as an expert C++ Systems Engineer. I am building the 'Network Guard' module for a ransomware-resilient database in C++20. I will not use Asio or Boost; I am building the TCP server from scratch using Linux POSIX sockets and `epoll`. I need to build an asynchronous event loop and a custom thread pool using `std::thread`, `std::mutex`, and `std::condition_variable`. Because I have a strong background in Data Structures and Algorithms in C++ and a foundational understanding of Go, you should map these complex C++ threading concepts to Go's goroutines and channels when explaining them to me. Let's start by designing the `TcpServer` class and the `epoll` loop structure to handle multiple non-blocking client connections simultaneously without crashing."

---

## Member 2: Security & Defense Engine (Tripwires & Lockdown)
**Goal:** Build the active defense systems that catch ransomware behavior in real-time and transition the database into lockdown before files are destroyed. 

**Core Responsibilities:**
*   **Change-Rate Detection:** Track write speeds to catch unnatural spikes in data modification (e.g., 15,000 updates/minute).
*   **Entropy Detection:** Calculate the Shannon entropy of incoming values to determine if the data looks scrambled/encrypted (e.g., `f8A#2kL91@x...`).
*   **Canary Records:** Manage hidden database entries (like `__CANARY_001__`) and trigger an alarm if they are modified.
*   **State Machine:** Use the State Pattern to seamlessly transition the database from `RUNNING` to `SUSPECTED` to `LOCKDOWN`.

**AI Prompt / Starting Context (Copy & Paste):**
> "Act as an expert C++ Cybersecurity Engineer. I am building the 'Security & Defense Engine' for a ransomware-resilient database in C++20. My module receives parsed network requests and evaluates them before they reach memory. I need to implement Object-Oriented design patterns to handle three main threats: calculating Shannon entropy for incoming strings, tracking the rate of requests over a sliding time window, and monitoring hidden 'Canary' records. I also need to build a state machine (Running, Suspected, Lockdown) using the State Pattern. Let's start by designing the abstract interfaces for a `SecurityFilter` using the Chain of Responsibility pattern, and the math required for real-time entropy calculation."

---

## Member 3: Memory Engine (Core Data Structures)
**Goal:** Build the lightning-fast, in-memory storage structure that holds the active data. You are prioritizing version history over overwriting.

**Core Responsibilities:**
*   Implement a highly efficient `std::unordered_map` (or `std::map`) where the keys map to custom "Version Chains" (a linked list of every change made to that key) rather than a single string.
*   Process standard database commands (`PUT`, `GET`, `DELETE`).
*   Implement a `HISTORY` command that retrieves older versions of a modified record based on timestamps.
*   Ensure thread-safe reads and writes, as multiple network clients will hit this memory structure simultaneously.

**AI Prompt / Starting Context (Copy & Paste):**
> "Act as an expert C++ Database Architect. I am building the 'Memory Engine' for a ransomware-resilient database in C++20. This database never overwrites data; it is append-only. I need to build an in-memory data structure centered around `std::unordered_map<std::string, VersionChain>`. The `VersionChain` needs to act as a linked list or vector of historical values for a single key, attached to timestamps. I need to safely process `PUT`, `GET`, `DELETE`, and `HISTORY` commands. Let's start by designing the core OOP classes for `Record`, `VersionChain`, and the main `MemoryStore`, ensuring we set up the correct abstractions for thread-safe access."

---

## Member 4: Disk Storage & Recovery (File I/O & Persistence)
**Goal:** Ensure zero data loss by safely writing every command to a permanent, append-only log on the hard drive, and building the recovery system to resurrect the database after a crash or attack.

**Core Responsibilities:**
*   Design a custom binary record layout (e.g., Type, Key, Value, Timestamp, Checksum).
*   Use `std::fstream` and `<filesystem>` to safely append records to the disk.
*   **Crash Recovery:** Read the binary log line-by-line on boot and reconstruct the Memory Engine's state.
*   **Point-in-Time Recovery:** Build the algorithm that allows an administrator to roll the database back to a specific timestamp, ignoring any corrupted records appended after an attack began.

**AI Prompt / Starting Context (Copy & Paste):**
> "Act as an expert C++ Storage Engineer. I am building the 'Disk Storage & Recovery' module for a ransomware-resilient database in C++20. My job is to manage permanent persistence using an append-only architecture. I need to design a custom binary file format that packs a command type, key, value, timestamp, and a checksum into bytes and safely writes them to disk using `std::fstream`. I also need to build a replay engine that reads this binary file on boot to reconstruct the database's memory state, including rolling back to specific timestamps to ignore ransomware-corrupted data. Let's start by designing the C++ binary struct layout and the safest RAII methods for continuous file appending."