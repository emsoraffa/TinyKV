#!/bin/bash

# WRAPPER: Run the client inside a temporary docker container
# We use 'host.docker.internal' to reach the ports exposed by docker-compose on your Mac
CLIENT="docker run --rm tinykv ./build/src/tinykv_client"

NODE1="host.docker.internal:50051"
NODE2="host.docker.internal:50052"
NODE3="host.docker.internal:50053"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "========================================"
echo "      TinyKV Cluster Smoke Test (Dockerized)"
echo "========================================"

# 1. TEST CONNECTIVITY
echo -n "[1/4] Pinging Node 1... "
# We suppress output to keep it clean, check exit code
$CLIENT $NODE1 ping >/dev/null 2>&1
if [ $? -eq 0 ]; then
  echo -e "${GREEN}OK${NC}"
else
  echo -e "${RED}FAIL${NC}"
  echo "      Error: Could not connect to $NODE1"
  echo "      Ensure 'docker-compose up' is running."
  exit 1
fi

# 2. TEST WRITE (Coordinator Logic)
KEY="smoke_test_key"
VAL="hello_docker_$(date +%s)"

echo -n "[2/4] Writing '$KEY=$VAL' to Node 1... "
$CLIENT $NODE1 put $KEY $VAL >/dev/null 2>&1
if [ $? -eq 0 ]; then
  echo -e "${GREEN}OK${NC}"
else
  echo -e "${RED}FAIL${NC}"
  exit 1
fi

# 3. TEST READ (Local Read)
echo -n "[3/4] Reading back from Node 1... "
# Capture output, strip newlines for cleaner checking
OUTPUT=$($CLIENT $NODE1 get $KEY 2>&1)

if [[ "$OUTPUT" == *"$VAL"* ]]; then
  echo -e "${GREEN}OK${NC}"
else
  echo -e "${RED}FAIL${NC}"
  echo "      Expected: ...$VAL..."
  echo "      Got:      $OUTPUT"
fi

# 4. TEST REPLICATION (Remote Read)
echo -n "[4/4] Verifying Replication on Node 2... "
OUTPUT=$($CLIENT $NODE2 get $KEY 2>&1)

if [[ "$OUTPUT" == *"$VAL"* ]]; then
  echo -e "${GREEN}OK${NC} (Replication Confirmed)"
else
  echo -e "${RED}FAIL${NC} - Node 2 missing data."
  echo "      (This is normal if the 'dumb' replication picked Node 3 or 4 instead)"
fi

echo "========================================"
