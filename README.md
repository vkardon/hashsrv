# High-Performance SHA-256 TCP Server

A C++ asynchronous TCP server built with Asio and OpenSSL. This project calculates SHA-256 hashes for line-terminated strings, utilizing a multi-threaded architecture designed for high throughput and memory efficiency.

## Key Features

* Asynchronous Multi-Threading: Distributes network I/O and hash calculations across all available CPU cores using a thread-pooled io_context.
* Streaming Hash Architecture: Processes data chunks as they arrive via OpenSSL's EVP interface. This allows processing of streams of infinite length without increasing RAM usage.
* Packet Fragmentation Recovery: Advanced Tail Buffer logic ensures that if a message is split across multiple TCP packets, or multiple messages arrive in one packet, the stream is correctly reassembled.
* High-Concurrency Stress Client: Includes a low-level Berkeley socket stress-test client capable of simulating thousands of simultaneous requests.
* Production Ready: Supports graceful shutdowns (SIGINT, SIGTERM) and handles both Linux (\n) and Windows (\r\n) line endings.

---

## Build and Run

### Prerequisites
* OS: Ubuntu 24.04 or compatible Linux (Alma/RHEL)
* Dependencies: build-essential (or Development Tools), cmake, libasio-dev, libssl-dev, libgtest-dev

### Local Compilation and Execution (from project root)

**1. Configure and Build:**  
   cmake -B build -DCMAKE_BUILD_TYPE=Release  
   cmake --build build -j$(nproc)  

**2. Run Unit and Integration Tests:**  
   ./build/server/session_test  
   ./build/server/server_test  

**3. Start the Server:**  
   ./build/server/server 8080  

**4. Run the Stress Test Client (in a separate terminal):**  
   ./build/client/client 127.0.0.1 8080  

---

## Containerization and Orchestration

The project uses a multi-stage Dockerfile to produce a runtime image.

### Build and Run with Docker

**1. Build images from scratch via Compose:**  
   docker compose build --no-cache  

**2. Reclaim disk space (Optional):**  
   docker image prune -f  

**3. Run Unit Tests (Session Logic):**  
   docker run --rm --entrypoint ./session_test hashsrv_server  

**4. Run Unit Tests (Server Logic):**  
   docker run --rm --entrypoint ./server_test hashsrv_server  

**5. Run System Integration Test:**  
    docker compose up --abort-on-container-exit --exit-code-from client  

**6. Start the server (Standalone):**  
    docker compose up server  
   
---

## Development & Automation (Scripts)

A suite of Bash scripts is provided in the `scripts/` directory to automate the development lifecycle and verify system integrity.

### CI/CD & Cleanup
* **`build_and_test.sh`**  
  Performs a "factory reset" build. It wipes all caches, builds images from scratch, and runs the entire test suite inside containers.  
  **Usage:** `./scripts/build_and_test.sh`  

* **`docker_clean.sh`**  
  Deep-cleans the environment by removing all containers, project images, and volumes.  
  **Usage:** `./scripts/docker_clean.sh`

### Functional Verification
* **`verify_individual.sh`**  
  Opens a **new connection for every line** to verify server accuracy against the system's native `sha256sum`.  
  **Usage:** `./scripts/verify_individual.sh -p 8080`  

* **`verify_multiple.sh`**  
  Validates **Stream & Chunking Integrity**: It sends a high-speed continuous stream over a single connection to ensure the server correctly identifies message boundaries.  
  **Usage:** `./scripts/verify_multiple.sh -p 8080 -f test_data.txt`  

---

## Comprehensive Testing

The project employs a multi-layered testing strategy to ensure that high performance never comes at the cost of data integrity.

### 1. Protocol & Logic Verification (`session_test`)  
Focuses on the core `Session` engine and the "Tail Buffer" logic.  
* **Fragmentation Recovery:**  
  Verifies that messages arriving byte-by-byte (the "slow drip" scenario) are correctly reassembled.  
* **Packet Congestion:**  
  Ensures that multiple messages arriving in a single TCP frame (`Msg1\nMsg2\n`) are both processed without data loss.  
* **Cryptographic Integrity:**
  Validates SHA-256 outputs against known test vectors using a `SessionTestWrapper` to isolate streaming logic.  

### 2. Infrastructure & Concurrency (`server_test`)  
Validates the thread-pooled `io_context` and the server's stability under pressure.  
* **Thundering Herd Scenario:**  
  Spawns 10x more concurrent clients than available CPU threads to verify thread safety and lock-free execution.  
* **Graceful Lifecycle:**  
  Confirms the server handles `SIGINT` and `SIGTERM` correctly, ensuring all socket buffers are flushed and threads are joined before exit.  

### 3. Integration Stress Test (`client`)  
A real-world simulation of massive network load.  
* **High-Throughput Pressure:**  
  Orchestrates 500 threads making 500 calls each (250,000 total requests) to monitor for memory leaks, socket exhaustion, or race conditions under sustained high-speed I/O.  

---

## Project Structure

| Component | Description |
| :--- | :--- |
| **server/** | C++ source for the async TCP server and session logic. |
| **server/tests/** | GoogleTest source files for unit and integration testing. |
| **client/** | C++ source for the high-concurrency stress test client. |
| **scripts/** | Development, automation, and verification utilities. |
| **docker/** | Multi-stage Dockerfiles and container configuration. |

---
