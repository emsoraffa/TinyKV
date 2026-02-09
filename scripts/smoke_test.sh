#!/bin/bash
# scripts/smoke_test.sh

# Run inside the network so we can reach nodes by name
CLIENT="docker run --rm --network tinykv-net tinykv:latest ./build/src/tinykv_client"
NODE1="tinykv-node1:50051"
NODE2="tinykv-node2:50052"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "========================================"
echo "      TinyKV Quorum Smoke Test"
echo "========================================"

echo -n "[1/4] Pinging Node 1... "
$CLIENT $NODE1 ping >/dev/null 2>&1
if [ $? -eq 0 ]; then echo -e "${GREEN}OK${NC}"; else
  echo -e "${RED}FAIL${NC}"
  exit 1
fi

KEY="smoke_key_$(date +%s)"
VAL="smoke_val"

echo -n "[2/4] Writing '$KEY=$VAL' (RF=3)... "
$CLIENT $NODE1 put $KEY $VAL 3 >/dev/null 2>&1
if [ $? -eq 0 ]; then echo -e "${GREEN}OK${NC}"; else
  echo -e "${RED}FAIL${NC}"
  exit 1
fi

echo -n "[3/4] Reading (R=3) from Node 1... "
OUTPUT=$($CLIENT $NODE1 get $KEY 3 2>&1)
if [[ "$OUTPUT" == *"$VAL"* ]]; then echo -e "${GREEN}OK${NC}"; else echo -e "${RED}FAIL${NC} (Got: $OUTPUT)"; fi

echo -n "[4/4] Reading via Node 2 (Forwarding)... "
OUTPUT=$($CLIENT $NODE2 get $KEY 2 2>&1)
if [[ "$OUTPUT" == *"$VAL"* ]]; then echo -e "${GREEN}OK${NC}"; else echo -e "${RED}FAIL${NC}"; fi
echo "========================================"
