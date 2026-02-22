# CMake toolchain file for MinGW-w64 cross-compilation from Linux to Windows
# Usage: cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compiler paths.
# Use the posix-threading variant: the win32 variant has a crippled C++ stdlib
# (no <ctime> clock_t, no std::thread, etc.) which causes Catch2 / test builds
# to fail.  The posix variant is installed alongside win32 by the mingw-w64
# package and is fully C++11/14 compliant.
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Where to find the target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Critical: Skip adding host system include paths during cross-compilation
# This prevents the compiler from finding Linux headers like /usr/include/wchar.h
set(CMAKE_INCLUDE_FLAG_CXX "-I")
set(CMAKE_INCLUDE_FLAG_C "-I")
set(CMAKE_DISABLE_SYSROOT_PATHS TRUE)

# Use sysroot to ensure we only use MinGW headers
set(CMAKE_SYSROOT /usr/x86_64-w64-mingw32)

# Windows-specific settings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# Disable RPATH
set(CMAKE_SKIP_RPATH TRUE)

# Force pkg-config to use cross-compilation paths
set(PKG_CONFIG_EXECUTABLE /usr/bin/pkg-config)
set(PKG_CONFIG_SYSROOT_DIR /usr/x86_64-w64-mingw32)
set(PKG_CONFIG_PREFIX /usr/x86_64-w64-mingw32)
