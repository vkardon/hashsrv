# STAGE 1: Build the binary
FROM ubuntu:24.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libasio-dev \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build_context
COPY CMakeLists.txt .
COPY server/ ./server/
COPY client/ ./client/

# Generate build files:
# Explicitly build the server and the two test binaries
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# STAGE 2: Create the lean runtime image
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y libssl3 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
# Copy ONLY the server binary
COPY --from=builder /build_context/build/server/server .
COPY --from=builder /build_context/build/server/server_test .
COPY --from=builder /build_context/build/server/session_test .

# The server listens on 8080
EXPOSE 8080
ENTRYPOINT ["./server", "8080"]

