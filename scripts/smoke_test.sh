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
echo "      TinyKV Quorum Smoke Test"
echo "========================================"

# 1. TEST CONNECTIVITY
echo -n "[1/4] Pinging Node 1... "
$CLIENT $NODE1 ping >/dev/null 2>&1
if [ $? -eq 0 ]; then
  echo -e "${GREEN}OK${NC}"
else
  echo -e "${RED}FAIL${NC}"
  echo "      Error: Could not connect. Is docker-compose up?"
  exit 1
fi

# 2. TEST WRITE (Standard RF=3)
KEY="quorum_key"
VAL="quorum_val_$(date +%s)"

echo -n "[2/4] Writing '$KEY=$VAL' to Node 1... "
$CLIENT $NODE1 put $KEY $VAL 3 >/dev/null 2>&1
if [ $? -eq 0 ]; then
  echo -e "${GREEN}OK${NC}"
else
  echo -e "${RED}FAIL${NC}"
  exit 1
fi

# 3. TEST QUORUM READ (R=3)
# Asking Node 1 to get consensus from 3 nodes.
echo -n "[3/4] Quorum Read (R=3) from Node 1... "
OUTPUT=$($CLIENT $NODE1 get $KEY 3)

# Filter out the [CLI] logs from stderr, just check if our value is in the output
if [[ "$OUTPUT" == *"$VAL"* ]]; then
  echo -e "${GREEN}OK${NC}"
else
  echo -e "${RED}FAIL${NC}"
  echo "      Expected: $VAL"
  echo "      Got:      $OUTPUT"
fi

# 4. TEST FORWARDING + QUORUM
# Ask Node 2 (who might not be owner) to coordinate a Quorum Read (R=2)
echo -n "[4/4] Forwarded Quorum Read (R=2) via Node 2... "
OUTPUT=$($CLIENT $NODE2 get $KEY 2)

if [[ "$OUTPUT" == *"$VAL"* ]]; then
  echo -e "${GREEN}OK${NC}"
else
  echo -e "${RED}FAIL${NC}"
  echo "      Expected: $VAL"
  echo "      Got:      $OUTPUT"
fi

echo "========================================"
