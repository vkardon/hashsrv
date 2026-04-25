# 🚀 High-Performance SHA-256 TCP Server

A C++ asynchronous TCP server built with Asio and OpenSSL. This project calculates SHA-256 hashes for line-terminated strings, utilizing a multi-threaded architecture designed for high throughput and memory efficiency.

## ✨ Key Features

* Asynchronous Multi-Threading: Distributes network I/O and hash calculations across all available CPU cores using a thread-pooled io_context.
* Streaming Hash Architecture: Processes data chunks as they arrive via OpenSSL's EVP interface. This allows processing of streams of infinite length without increasing RAM usage.
* Packet Fragmentation Recovery: Advanced Tail Buffer logic ensures that if a message is split across multiple TCP packets, or multiple messages arrive in one packet, the stream is correctly reassembled.
* High-Concurrency Stress Client: Includes a low-level Berkeley socket stress-test client capable of simulating thousands of simultaneous requests.
* Production Ready: Supports graceful shutdowns (SIGINT, SIGTERM) and handles both Linux (\n) and Windows (\r\n) line endings.

---

## 🏗 Build and Run

### Prerequisites
* OS: Ubuntu 22.04 or 24.04
* Dependencies: build-essential, cmake, libasio-dev, libssl-dev, libgtest-dev

### Local Compilation and Execution
1. Build the project:
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j$(nproc)

2. Start the Server:
   ./server/server 8080

3. Run the Stress Test Client:
   ./client/client 127.0.0.1 8080

---

## 🧪 Comprehensive Testing

The project includes an extensive test suite that validates both the network infrastructure and the core logic.

### 1. Logic and Protocol Tests (session_test)
* Fragmentation: Verifies that messages arriving byte-by-byte are correctly reassembled.
* Multi-Message Packets: Ensures that if Msg1\nMsg2 arrives in one read, both are processed.
* Integrity: Validates the SHA-256 output against known test vectors using a SessionTestWrapper.

### 2. Infrastructure Tests (server_test)
* Thundering Herd: Spawns 10x more clients than CPU threads to verify thread safety and performance under pressure.
* Lifecycle: Confirms the server starts, accepts connections, and shuts down gracefully via signals.

### 3. Integration Stress Test (client)
The custom client simulates a massive load (500 threads making 500 calls each) across multiple runs to ensure the server remains stable and memory-efficient under real-world networking conditions.

---

## 🐋 Containerization and Orchestration

The project uses a multi-stage Dockerfile to produce a minimal runtime image (~100MB) by stripping away build tools.

### Build and Run with Docker
1. Build the standalone image:
   docker build -t hashsrv .

2. Run the full stack via Compose:
   docker compose up --build

---

## 📂 Project Structure

| Component | Description |
| :--- | :--- |
| server/server.hpp | Thread-pool management and async acceptor logic. |
| server/session.hpp | Core engine: Async R/W, streaming hashing, and tail-buffer management. |
| client/client.cpp | Multi-threaded stress-test tool using Berkeley sockets. |
| include/utils.hpp | RAII Profiling, Hex-LUT conversion, and logging macros. |
| tests/ | GTest suites for unit and integration testing. |
| docker/ | Multi-stage Dockerfiles for optimized deployments. |

---
