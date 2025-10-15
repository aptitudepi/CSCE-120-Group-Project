
FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-declarative-dev \
    libqt6network6 \
    libqt6sql6 \
    libqt6sql6-sqlite \
    libqt6sql6-psql \
    libqt6positioning6 \
    libqt6location6 \
    libqt6httpserver6-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source code
COPY . .

# Build the services
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Create runtime directory structure
RUN mkdir -p /app/runtime/bin && \
    mkdir -p /app/runtime/etc && \
    cp build/services/*/bin/* /app/runtime/bin/ || true && \
    cp build/frontend/HyperlocalWeatherApp /app/runtime/bin/ || true

WORKDIR /app/runtime

# Default command (will be overridden in docker-compose)
CMD ["./bin/ApiGatewayService"]
