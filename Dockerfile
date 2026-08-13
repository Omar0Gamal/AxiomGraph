# Build Stage
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3 \
    python3-dev \
    python3-pip \
    libomp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Build the C++ Server
WORKDIR /app/build
RUN cmake .. && cmake --build . --target server_http

# Runtime Stage
FROM ubuntu:22.04

# Install runtime dependencies (OpenMP for multithreading)
RUN apt-get update && apt-get install -y \
    libomp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled executable and sqlite embedded library
COPY --from=builder /app/build/server_http /usr/local/bin/axiomgraph-server

# Expose HTTP API Port
EXPOSE 8000

# Directory for storing database persistent files
VOLUME /data
WORKDIR /data

# Start the server
CMD ["axiomgraph-server"]
