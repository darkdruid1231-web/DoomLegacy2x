#!/bin/bash

# CI script for Windows MinGW build (equivalent to GitHub Actions windows-msys2 job)
# This is for cross-compilation on Linux, using MinGW
# Run from doomlegacy-legacy2 directory

set -e

echo "Starting Windows MinGW CI build..."

# Install dependencies if needed (on Ubuntu/Debian)
# sudo apt-get update
# sudo apt-get install -y cmake mingw-w64 g++-mingw-w64

# Create build directory
mkdir -p build-mingw
cd build-mingw

# Configure with CMake for MinGW cross-compilation
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SDL2=OFF -DCMAKE_TOOLCHAIN_FILE=../legacy/trunk/cmake/mingw-w64-x86_64.cmake

# Build
make -j$(nproc)

# Run tests (cross-compiled executables)
echo "Running Windows tests..."
wine ./test_fixed_t.exe
wine ./test_fixed_catch2.exe
wine ./test_vector.exe
wine ./test_console.exe

echo "Windows MinGW CI build completed successfully!"