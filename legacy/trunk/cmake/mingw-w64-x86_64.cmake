# CMake toolchain file for MinGW-w64 cross-compilation from Linux to Windows
# Usage: cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compiler paths.
# Use the posix-threading variant: the win32 variant ships with a crippled
# C++ stdlib that breaks <ctime>/<chrono> (clock_t not in global namespace).
# The posix variant is installed alongside win32 by the mingw-w64 package.
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# Tell CMake's find_* commands to look inside the MinGW sysroot for libraries
# and headers, and to never search the build-host programs.
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Do NOT set CMAKE_SYSROOT here.  Setting it passes --sysroot to the compiler
# which disrupts the C++ stdlib's #include_next chain and breaks <ctime> /
# <chrono> when they try to pull clock_t / tm from the system time.h.
# The MinGW cross-compiler already knows its own sysroot path internally.

# Windows-specific link flags: bundle libgcc/libstdc++ statically so the
# produced .exe doesn't require extra DLLs at runtime.
set(CMAKE_C_FLAGS_INIT   "-Wno-missing-attributes")
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++ -Wno-missing-attributes")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# No RPATH on Windows
set(CMAKE_SKIP_RPATH TRUE)

# pkg-config: point at the MinGW sysroot's .pc files.
# (PKG_CONFIG_PATH env var is also set in the CI step for redundancy.)
set(PKG_CONFIG_EXECUTABLE    /usr/bin/pkg-config)
set(PKG_CONFIG_SYSROOT_DIR   /usr/x86_64-w64-mingw32)
set(PKG_CONFIG_PREFIX        /usr/x86_64-w64-mingw32)
