# CMake toolchain file for Clang cross-compilation to MinGW from Linux
# Usage: cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/clang-mingw-w64-x86_64.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compiler: use Clang targeting MinGW
set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# Tell CMake's find_* commands to look inside the MinGW sysroot for libraries
# and headers, and to never search the build-host programs.
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Clang target and linker flags for MinGW cross-compilation
set(CMAKE_C_FLAGS   "-target x86_64-w64-mingw32 -I/usr/x86_64-w64-mingw32/include -L/usr/x86_64-w64-mingw32/lib -L/usr/lib/gcc/x86_64-w64-mingw32/13-posix")
set(CMAKE_CXX_FLAGS "-target x86_64-w64-mingw32 -I/usr/x86_64-w64-mingw32/include -L/usr/lib/gcc/x86_64-w64-mingw32/13-posix -L/usr/x86_64-w64-mingw32/lib")

# Windows-specific link flags: bundle libgcc/libstdc++ statically so the
# produced .exe doesn't require extra DLLs at runtime.
# Disabled for Clang - causes linking issues
# set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -static-libgcc -static-libstdc++")

# No RPATH on Windows
set(CMAKE_SKIP_RPATH TRUE)

# pkg-config: point at the MinGW sysroot's .pc files.
set(PKG_CONFIG_EXECUTABLE    /usr/bin/pkg-config)
set(PKG_CONFIG_SYSROOT_DIR   /usr/x86_64-w64-mingw32)
set(PKG_CONFIG_PREFIX        /usr/x86_64-w64-mingw32)