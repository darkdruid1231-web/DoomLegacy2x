#!/bin/bash

# CI script for Linux build (equivalent to GitHub Actions linux-build job)
# Run from doomlegacy-legacy2 directory

set -e

echo "Starting Linux CI build..."

# Install dependencies if needed
# sudo apt-get update
# sudo apt-get install -y cmake clang clang-tidy clang-format

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SDL2=ON

# Build
make -j$(nproc)

# Run clang-tidy on specific files
echo "Running clang-tidy..."
clang-tidy ../legacy/trunk/engine/g_state.cpp ../legacy/trunk/net/n_connection.cpp -p .

# Check clang-format
echo "Checking clang-format..."
find ../legacy/trunk -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror

# Run tests
echo "Running tests..."
./test_fixed_t
./test_fixed_catch2
./test_vector
./test_console

echo "Linux CI build completed successfully!"