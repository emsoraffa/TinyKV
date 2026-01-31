#!/bin/bash

CLIENT="docker run --rm tinykv ./build/src/tinykv_client"
NODE1="host.docker.internal:50051" # Alive Node
NODE2="host.docker.internal:50052" # Node we will KILL

KEY="survival_key"
VAL="I_WILL_SURVIVE"

echo "=========================================="
echo "    TinyKV Resilience Demo"
echo "=========================================="

# 1. Write Data (Replicates to Node 1, 2, and maybe others)
echo -n "[1/4] Writing Data to Cluster... "
$CLIENT $NODE1 put $KEY $VAL 3 >/dev/null 2>&1
echo "OK"

# 2. Kill Node 2
echo -n "[2/4] Simulating Crash (Killing Node 2)... "
docker-compose stop node2 >/dev/null 2>&1
echo "KILLED"

# 3. Read with Quorum (R=2)
# We ask Node 1. Node 1 will try to contact Node 2 (Dead) and Node X (Alive).
# Since your code handles failures by returning {-1}, the Priority Queue
# will effectively filter out the dead node and return the data from the survivor.
echo -n "[3/4] Reading with Quorum R=2... "

OUTPUT=$($CLIENT $NODE1 get $KEY 2 2>&1) # Capture stderr too just in case

if [[ "$OUTPUT" == *"$VAL"* ]]; then
  echo "SUCCESS!"
  echo "      Retrieved: $VAL"
else
  echo "FAILED"
  echo "      Got: $OUTPUT"
fi

# 4. Restore
echo -n "[4/4] Healing Cluster (Restarting Node 2)... "
docker-compose start node2 >/dev/null 2>&1
echo "DONE"
echo "=========================================="
