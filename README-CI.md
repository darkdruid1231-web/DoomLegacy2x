# Doom Legacy CI/CD Pipeline

This document describes the consistent local/CI pipeline for Doom Legacy testing and building.

## Overview

The pipeline provides:
- **Consistent builds** across local development and CI
- **Containerized builds** for CI consistency
- **Unified scripts** that work locally and in GitHub Actions
- **Quick local feedback** via task runner (Makefile)
- **Single source of dependencies**

## Quick Start

### Local Development

```bash
# Install dependencies
make deps

# Build and test
make build test

# Or use individual commands
make build          # Build for Linux
make build-windows  # Cross-compile for Windows
make test           # Run tests
make clean          # Clean builds
```

### Using Containers (Recommended for CI consistency)

```bash
# Build in container
make docker-build

# Test in container
make docker-test
```

## Architecture

### Core Components

- **`build.sh`** - Unified build script (local + CI)
- **`test.sh`** - Unified test script (local + CI)
- **`Makefile`** - Task runner for quick local feedback
- **`Dockerfile.linux`** - Container for Linux builds
- **`Dockerfile.windows`** - Container for Windows cross-compilation
- **`dependencies.txt`** - Single source of truth for dependencies

### Build Targets

| Target | Description | Use Case |
|--------|-------------|----------|
| `linux` | Native Linux build with GCC | Primary development |
| `windows` | Windows cross-compilation | Windows binaries |
| `clang` | Linux build with Clang | Compiler validation |

### Build Types

- **Release** - Optimized builds for distribution
- **Debug** - Debug symbols for development/testing

## Local Development Workflow

### First Time Setup

```bash
# Clone repository
git clone <repository>
cd doomlegacy-legacy2

# Install dependencies
make deps

# Build and test
make all
```

### Daily Development

```bash
# Quick iteration: build and run tests
make test

# Full validation: all targets
make all

# Clean rebuild
make clean build test
```

### Advanced Usage

```bash
# Debug build
make build-debug

# Cross-compile for Windows
make build-windows

# Run only quick tests
make test-quick

# Format code
make format

# Run linting
make lint
```

## CI Pipeline

The GitHub Actions workflow uses the same scripts as local development:

### Jobs

- **`linux-build`** - Linux GCC build with tests
- **`windows-build`** - Windows cross-compilation
- **`clang-build`** - Linux Clang build for validation

### CI Commands

The CI runs these Makefile targets:
- `make ci-linux` - Linux build + test
- `make ci-windows` - Windows build + test
- `make ci-clang` - Clang build + test

## Container Usage

### Building Containers

```bash
# Build Linux container
docker build -f Dockerfile.linux -t doomlegacy-linux .

# Build Windows container
docker build -f Dockerfile.windows -t doomlegacy-windows .
```

### Running Builds in Containers

```bash
# Linux build
./build.sh --container -t linux

# Windows cross-compile
./build.sh --container -t windows
```

## Dependencies

### System Dependencies

See `dependencies.txt` for the complete list. Install with:

```bash
# Ubuntu/Debian
make deps

# Manual installation
sudo apt-get install -y $(cat dependencies.txt | grep -v '^#' | tr '\n' ' ')
```

### Container Dependencies

Dependencies are baked into the Docker images:
- `Dockerfile.linux` - Ubuntu with SDL1.2, CMake, GCC, Clang
- `Dockerfile.windows` - Ubuntu with MinGW cross-compiler, SDL2

## Troubleshooting

### Build Issues

```bash
# Clean rebuild
make clean build

# Verbose build
./build.sh -t linux -v

# Check build logs
tail -f legacy/trunk/build-linux/build.log
```

### Test Issues

```bash
# Run tests with verbose output
./test.sh --verbose

# Run specific test
./test.sh --quick  # Skip integration tests
```

### Container Issues

```bash
# Check container
docker run --rm doomlegacy-linux /bin/bash

# Debug build in container
docker run --rm -it doomlegacy-linux bash -c "cd legacy/trunk && ../build.sh -t linux"
```

## Customization

### Adding New Build Targets

1. Update `build.sh` to handle new target
2. Add Dockerfile if needed
3. Update Makefile targets
4. Update CI workflow

### Adding New Tests

1. Add test to CMakeLists.txt
2. Update `test.sh` to run new test
3. Update Makefile targets

### Dependency Changes

1. Update `dependencies.txt`
2. Update Dockerfiles
3. Update CI installation scripts

## Migration from Legacy Scripts

### Old Way
```bash
# Manual dependency installation
sudo apt-get install cmake libsdl1.2-dev ...

# Direct CMake
cd legacy/trunk && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release ...
make

# Run tests manually
./test_fixed_t
./test_vector
...
```

### New Way
```bash
# Single command
make deps build test

# Or with containers
make docker-build docker-test
```

## Performance

### Local Builds
- **Incremental builds** - Only rebuild changed files
- **Parallel compilation** - Uses all CPU cores
- **Quick feedback** - Makefile provides fast local iteration

### CI Builds
- **Container caching** - Docker layer caching
- **Dependency caching** - APT package caching
- **Parallel jobs** - Multiple build targets run concurrently

## Security

- **Container isolation** - Builds run in isolated containers
- **Dependency pinning** - Fixed versions in Dockerfiles
- **Minimal images** - Ubuntu base with only required packages
- **No privileged operations** - Builds run as regular user