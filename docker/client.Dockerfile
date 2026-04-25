# --- STAGE 1: Build ---
FROM ubuntu:24.04 AS builder
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake

WORKDIR /build_context
COPY client/ ./client/

# Build specifically from the client subdirectory
RUN cd client && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# --- STAGE 2: Runtime ---
FROM ubuntu:24.04
WORKDIR /app
# Note: Adjust path to where the binary actually ends up
COPY --from=builder /build_context/client/build/client .

ENTRYPOINT ["./client"]

