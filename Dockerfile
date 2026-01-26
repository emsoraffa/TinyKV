# Use a modern Linux version as the base
FROM ubuntu:22.04

# Avoid time zone prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# 1. Install dependencies (C++, CMake, gRPC, Protobuf)
# We use the package manager (apt) so we don't have to compile gRPC from source (which takes 20 mins)
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  git \
  pkg-config \
  libgrpc++-dev \
  protobuf-compiler-grpc \
  && rm -rf /var/lib/apt/lists/*

# 2. Copy your source code into the container
WORKDIR /app
COPY . .

# 3. Build the project
# We create a build folder, run cmake, and compile
RUN mkdir -p build && cd build && \
  cmake .. && \
  make -j$(nproc)

# 4. By default, start a shell so we can run commands manually
CMD ["/bin/bash"]
