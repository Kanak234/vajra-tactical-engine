# Multi-stage build for Vajra Tactical Engine
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    ca-certificates \
    git \
    libsdl2-dev \
    libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source tree
COPY . .

# Configure and build
RUN cmake -B build_docker -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build_docker -j$(nproc)

# Run test suite
RUN ctest --test-dir build_docker --output-on-failure

# Production runner image
FROM ubuntu:22.04 AS runner

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    libsdl2-2.0-0 \
    libgl1 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build_docker/vajra /app/vajra
COPY --from=builder /app/build_docker/vajra_tests /app/vajra_tests
COPY --from=builder /app/assets /app/assets

# Create unprivileged user
RUN useradd -m -u 1000 vajra && chown -R vajra:vajra /app
USER vajra

ENTRYPOINT ["/app/vajra_tests"]
