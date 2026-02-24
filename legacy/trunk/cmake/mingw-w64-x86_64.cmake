# CMake toolchain file for MinGW-w64 cross-compilation from Linux to Windows
# Usage: cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compiler paths: use GCC MinGW
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# Tell CMake's find_* commands to look inside the MinGW sysroot for libraries
# and headers, and to never search the build-host programs.
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set sysroot to prevent host header leakage
set(CMAKE_SYSROOT /usr/x86_64-w64-mingw32)

# Windows-specific link flags: bundle libgcc/libstdc++ statically so the
# produced .exe doesn't require extra DLLs at runtime.
set(CMAKE_C_FLAGS_INIT   "-Wno-missing-attributes -Wno-error")
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++ -Wno-missing-attributes -Wno-error")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# No RPATH on Windows
set(CMAKE_SKIP_RPATH TRUE)

# pkg-config: point at the MinGW sysroot's .pc files.
set(PKG_CONFIG_EXECUTABLE    /usr/bin/pkg-config)
set(PKG_CONFIG_SYSROOT_DIR   /usr/x86_64-w64-mingw32)
set(PKG_CONFIG_PREFIX        /usr/x86_64-w64-mingw32)
