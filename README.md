# TinyKV: High-Performance Distributed Key-Value Store

TinyKV is a distributed, high-performance, partition-tolerant key-value store inspired by Amazon Dynamo. It implements a peer-to-peer architecture using consistent hashing, tunable consistency (quorums), and "Last Writer Wins" conflict resolution.
Built with Modern C++ (C++20) and gRPC.

---

## 🚀 Quick Start

The entire system is Dockerized to ensure reproducibility.

### Prerequisites

- Docker & Docker Compose
- Make (Optional)

### 1. Build and Start the Cluster

This will compile the C++ source and spin up a 5-node cluster:

```sh
make build
make up
```

### 2. Run Performance Benchmark

Runs a multi-threaded C++ benchmark (50 threads, 10,000 Ops):

```sh
make benchmark
```

**Expected Result:**  
~5,000 OPS (Operations Per Second) with <10ms latency.

### 3. Run Resilience Demo (Fault Tolerance)

This script writes data, kills a node container, and proves that Get requests still succeed using quorum reads:

```sh
make resilience
```

**Expected Result:**  
SUCCESS! Retrieved: `I_WILL_SURVIVE`

### 4. Functional Smoke Test

Runs a basic put/get verification script:

```sh
make test
```

### 5. Shutdown

```sh
make down
```

---

## 🏛 System Architecture

**Core Concepts implemented:**

- **Partitioning:**  
  Data is distributed across 5 nodes using consistent hashing with virtual nodes to ensure even load distribution. Please note that 5 is an arbitrary number, and the system scales with more nodes.

- **Replication:**  
  Every key is replicated to successors. The replication factor is provided as an argument via the commandline but is by default set to 3.

- **Tunable Consistency:**
  - **Write Path:** The coordinator forwards data to replicas.
  - **Read Path:** Supports quorum reads (`R + W > N`). The coordinator queries nodes, compares timestamps, and returns the "Last Writer Wins" version.

- **Failure Detection:**  
  Nodes maintain a heartbeat with peers. If a node is unreachable, requests are routed to the next available member in the preference list.

---

## 📂 Project Structure

```
.
├── config/clusters.txt      # Cluster network configuration
├── docker-compose.yml
├── protos/tinykv.proto     # gRPC Protocol Definitions
├── src/
│   ├── client/             # Client SDK & CLI implementation
│   ├── server/             # Server/Node Logic
│   │   ├── HashRing.cpp
│   │   └── Server.cpp
│   └── Utils.cpp
└── scripts/                # Test suites
```

---

## 🛠 Usage (Manual CLI)

You can manually interact with the cluster using the CLI tool.

### Enter the CLI container

```sh
docker run -it --rm --network tinykv-net tinykv \
  ./build/src/tinykv_client tinykv-node1:50051 <COMMAND> [ARGS]
```

### Commands

- `put <key> <val> [rf]`  
  Writes a value with specific Replication Factor.

- `get <key> [quorum]`  
  Reads a value using specific Quorum Size.

#### Example

**Write with Replication Factor 3:**

```sh
./tinykv_client tinykv-node1:50051 put my_key my_data 3
```

**Read with Quorum 2 (Strong Consistency):**

```sh
./tinykv_client tinykv-node1:50051 get my_key 2
```

---

## 📊 Performance

On a standard development machine (Docker Desktop), the system achieves:

- **Write Throughput:** ~2,000 - 5,000 OPS
- **Read Latency:** ~6ms (p99)

_Note: Performance is bounded by Docker network bridging overhead rather than C++ execution speed._
