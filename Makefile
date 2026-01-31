.PHONY: build up down logs shell benchmark resilience

# Build the docker image
build:
	docker-compose build

# Start the cluster in the background
up:
	docker-compose up -d
	@echo "Waiting for cluster to stabilize..."
	@sleep 5

# Stop the cluster
down:
	docker-compose down

# View logs
logs:
	docker-compose logs -f

# Run the performance benchmark with default values.
benchmark:
	@chmod +x scripts/benchmark.sh
	./scripts/benchmark.sh

# Run the resilience test
resilience:
	@chmod +x scripts/resilience_test.sh
	./scripts/resilience_test.sh

# Run a manual shell to play with the CLI
cli:
	docker run -it --rm --network tinykv-net tinykv ./build/src/tinykv_client tinykv-node1:50051 help

test:
	@chmod +x scripts/smoke_test.sh
	./scripts/smoke_test.sh
