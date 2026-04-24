# STEP 1: Use a temporary "Builder" image with all compilers
FROM ubuntu:22.04 AS builder

# Prevent prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install everything needed to compile your C++ code
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libasio-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /build_context

# Copy your source code into the container
COPY . .

# Compile the project
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# STEP 2: Create the final, tiny Runtime image
FROM ubuntu:22.04

# Install ONLY the runtime SSL libraries (no compilers)
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy only the compiled binary from the builder stage
# Replace 'hashsrv' with the actual name of your executable
COPY --from=builder /build_context/build/hashsrv .

# Tell Docker which port the server uses
EXPOSE 8080

# The command to run when the container starts
ENTRYPOINT ["./hashsrv", "8080"]