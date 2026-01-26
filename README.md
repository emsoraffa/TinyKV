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
