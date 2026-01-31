#!/bin/bash
# Runs the C++ benchmark tool inside the docker network
CLIENT="docker run --rm --network tinykv-net tinykv ./build/src/tinykv_client"
NODE="tinykv-node1:50051"

# 10000 Ops, RF=3, 50 Threads
$CLIENT $NODE benchmark 10000 3 50
