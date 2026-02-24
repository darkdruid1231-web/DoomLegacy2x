#!/bin/bash

# CI script for Clang MinGW build (equivalent to GitHub Actions windows-clang-build job)
# Alternative Windows build using Clang + MinGW
# Run from doomlegacy-legacy2 directory

set -e

echo "Starting Clang MinGW CI build..."

# Install dependencies if needed
# sudo apt-get update
# sudo apt-get install -y cmake clang llvm lld mingw-w64 zlib1g-dev

# Build zlib with cmake (to avoid configure issues)
echo "Building zlib..."
mkdir -p zlib-build
cd zlib-build
cmake ../zlib -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

# Create build directory
mkdir -p build-clang
cd build-clang

# Configure with CMake for Clang MinGW
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../legacy/trunk/cmake/clang-mingw-w64-x86_64.cmake -DZLIB_LIBRARY=../zlib-build/libz.a -DZLIB_INCLUDE_DIR=../zlib-build

# Build
make -j$(nproc)

# Run tests
echo "Running Clang MinGW tests..."
wine ./test_fixed_t.exe
wine ./test_fixed_catch2.exe
wine ./test_vector.exe
wine ./test_console.exe

echo "Clang MinGW CI build completed successfully!"