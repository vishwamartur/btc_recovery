# Bitcoin Wallet Recovery System Docker Image

# Multi-stage build for smaller final image
FROM nvidia/cuda:11.8-devel-ubuntu20.04 as builder

# Avoid interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    libboost-all-dev \
    opencl-headers \
    ocl-icd-opencl-dev \
    wget \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the application
RUN mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_GPU=ON \
        -DENABLE_CUDA=ON \
        -DENABLE_OPENCL=ON \
        -DBUILD_TESTS=OFF \
        -DBUILD_EXAMPLES=OFF && \
    make -j$(nproc)

# Runtime stage
FROM nvidia/cuda:11.8-runtime-ubuntu20.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl1.1 \
    libboost-system1.71.0 \
    libboost-filesystem1.71.0 \
    libboost-thread1.71.0 \
    ocl-icd-libopencl1 \
    && rm -rf /var/lib/apt/lists/*

# Create application user
RUN useradd -m -u 1000 btcrecovery

# Set working directory
WORKDIR /app

# Copy built application from builder stage
COPY --from=builder /app/build/btc-recovery /usr/local/bin/
COPY --from=builder /app/config /app/config/
COPY --from=builder /app/README.md /app/

# Create directories for data and logs
RUN mkdir -p /app/data /app/logs && \
    chown -R btcrecovery:btcrecovery /app

# Switch to application user
USER btcrecovery

# Set environment variables
ENV PATH="/usr/local/bin:${PATH}"
ENV CUDA_VISIBLE_DEVICES=all

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD btc-recovery --help > /dev/null || exit 1

# Default command
ENTRYPOINT ["btc-recovery"]
CMD ["--help"]

# Labels
LABEL maintainer="Bitcoin Recovery Team"
LABEL version="1.0.0"
LABEL description="Bitcoin Wallet Password Recovery System"
LABEL org.opencontainers.image.source="https://github.com/your-repo/btc-recovery"
LABEL org.opencontainers.image.documentation="https://github.com/your-repo/btc-recovery/blob/main/README.md"
LABEL org.opencontainers.image.licenses="MIT"

# Usage examples in comments:
# 
# Build image:
# docker build -t btc-recovery .
#
# Run with CPU only:
# docker run -v /path/to/wallet:/app/data btc-recovery \
#   --wallet /app/data/wallet.dat --charset mixed --min-length 6 --max-length 12
#
# Run with GPU support:
# docker run --gpus all -v /path/to/wallet:/app/data btc-recovery \
#   --wallet /app/data/wallet.dat --gpu --charset mixed
#
# Run with custom config:
# docker run -v /path/to/wallet:/app/data -v /path/to/config:/app/config btc-recovery \
#   --config /app/config/my_config.yaml
#
# Run dictionary attack:
# docker run -v /path/to/wallet:/app/data -v /path/to/dict:/app/dict btc-recovery \
#   --wallet /app/data/wallet.dat --dictionary /app/dict/passwords.txt
#
# Interactive shell:
# docker run -it --entrypoint /bin/bash btc-recovery
