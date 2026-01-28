# TinyKV

TinyKV is a distributed key-value store implementation using C++20 and gRPC. It supports data replication and tunable consistency on a static cluster.

## Prerequisites

- **Docker** (Required for building and running)
- **gRPC & Protobuf**

---

## 🛠️ Development Setup

We use Docker to ensure a consistent Linux build environment.

### 1. Build the Docker Image

Run this once to set up the build environment (Ubuntu + gRPC + C++ tools).

```bash
docker build -t tinykv .## How to use

TinyKV supports two different commands: put/get.

To use the put command use the following format

```

Each node maintains a hashmap datastructure of the status of the other nodes. By keeping track of when each node was last seen, and periodically pinging each node via a heartbeat a given node can know which nodes are up and which are not. To determine whether a node is down it simply checks if the last_seen timestamp is older than 15seconds from the current time. This datastructure has to be thread safe as any kind of request will be tracked, both from heartbeat pulses and regular requests.
