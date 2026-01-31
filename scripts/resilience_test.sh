#!/bin/bash

# We run the client INSIDE the 'tinykv-net' network.
# This allows us to talk to 'tinykv-node1' directly by name.
# This works on Mac, Linux, and Windows reliably.
CLIENT="docker run --rm --network tinykv-net tinykv ./build/src/tinykv_client"
NODE1="tinykv-node1:50051"

KEY="survival_key"
VAL="I_WILL_SURVIVE"

echo "=========================================="
echo "    TinyKV Resilience Demo"
echo "=========================================="

echo -n "[1/4] Writing Data to Cluster... "
$CLIENT $NODE1 put $KEY $VAL 3 >/dev/null 2>&1
echo "OK"

echo -n "[2/4] Simulating Crash (Killing Node 2)... "
docker stop tinykv-node2 >/dev/null 2>&1
echo "KILLED"

echo -n "[3/4] Reading with Quorum R=2... "
OUTPUT=$($CLIENT $NODE1 get $KEY 2 2>&1)

if [[ "$OUTPUT" == *"$VAL"* ]]; then
  echo "SUCCESS!"
  echo "      Retrieved: $VAL"
else
  echo "FAILED"
  echo "      Got: $OUTPUT"
fi

echo -n "[4/4] Healing Cluster (Restarting Node 2)... "
docker start tinykv-node2 >/dev/null 2>&1
echo "DONE"
echo "=========================================="
