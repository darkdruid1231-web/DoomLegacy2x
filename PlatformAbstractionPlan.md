# Platform Abstraction Plan for Doom Legacy

## Overview

This document outlines the platform-specific code issues found in Doom Legacy and provides a phased plan to make the software build and run on Windows, Linux, and macOS.

---

## 1. Files with Platform-Specific Issues

### 1.1 Core Type Definitions

| File | Issues |
|------|--------|
| `include/core/doomtype.h` | Platform detection via `__WIN32__`, `__APPLE_CC__`, `__linux__`, `__MSC__`, `__OS2__`, `FREEBSD`. O_BINARY handling. Windows-specific `__int64` vs POSIX `int64_t`. |

### 1.2 System Interface

| File | Issues |
|------|--------|
| `interface/sdl/i_system.cpp` | Hardcoded Linux (`LINUX` #ifdef), Windows (`__WIN32__`), Apple (`__APPLE_CC__`). `/dev/gpmdata` and `/dev/ttyS*` device paths. `I_GetFreeMem()` reads `/proc/meminfo` (Linux-only). `I_GetDiskFreeSpace()` has separate implementations for each platform. |
| `engine/d_main.cpp` | Uses `#ifdef LINUX` for home directory handling. Relies on `$HOME` environment variable. |

### 1.3 Sound Interface

| File | Issues |
|------|--------|
| `include/i_sound.h` | `#ifdef __MACOS__` changes `I_RegisterSong()` function signature. |

### 1.4 DLL Loading

| File | Issues |
|------|--------|
| `include/m_dll.h` | `#ifdef __WIN32__` for DLL handle type and export macros. |
| `util/m_dll.cpp` | Separate Windows (`LoadLibrary`) and POSIX (`dlopen`) implementations. |

### 1.5 Video/Rendering

| File | Issues |
|------|--------|
| `video/hardware/hwr_geometry.cpp` | `#ifdef __WIN32__` for geometry code. |
| `video/r_main.cpp` | Comment referencing WIN32 port work. |

### 1.6 Console/Utilities

| File | Issues |
|------|--------|
| `util/console.cpp` | `#if !defined(SDL) && (defined(__WIN32__) || defined(__OS2__))` for non-SDL platforms. |
| `util/w_wad.cpp` | `#ifndef __WIN32__` for file operations. |
| `engine/menu/menu.cpp` | `#ifdef __WIN32__` for menu handling. |
| `engine/singleplayer_stubs.cpp` | `#ifdef _WIN32` stubs. |

### 1.7 Build System

| File | Issues |
|------|--------|
| `CMakeLists.txt` | Hardcoded paths: `/home/geoffrey/tnl/lib/libtnl.a`, `/home/geoffrey/tnl/lib/libtomcrypt.a`, `/home/geoffrey/tnl/tnl/tnl.h`, `/home/geoffrey/tnl/include`. ENet search paths: `/usr`, `/usr/local`, `C:/enet-mingw`, `C:/Program Files/enet`. Hardcoded `linux`, `i386`, `__GNUC__` compile definitions. |

---

## 2. Specific Problems Found

### 2.1 Hardcoded Library Paths

```cmake
# CMakeLists.txt - Lines with hardcoded paths:
/home/geoffrey/tnl/lib/libtnl.a      # TNL library
/home/geoffrey/tnl/lib/libtomcrypt.a # Tomcrypt library  
/home/geoffrey/tnl/tnl/tnl.h         # TNL headers
/home/geoffrey/tnl/include           # TNL include path
/usr, /usr/local                     # ENet search paths (Linux)
C:/enet-mingw, C:/Program Files/enet # ENet search paths (Windows)
```

### 2.2 Hardcoded Device Paths

```cpp
// interface/sdl/i_system.cpp
"/dev/gpmdata"  // Linux mouse device
"/dev/ttyS0"    // Serial port 0
"/dev/ttyS1"    // Serial port 1
"/dev/ttyS2"    // Serial port 2
"/dev/ttyS3"    // Serial port 3
```

### 2.3 Platform Preprocessor Defines

The following defines are hardcoded in CMakeLists.txt:

```cmake
# Hardcoded compile definitions
FN_CDECL=
i386         # x86 architecture only
linux        # Linux-only
__GNUC__     # GCC-specific
QSORT_CALLBACK=
TNL_LITTLE_ENDIAN
_TNL_X86_    # TNL x86-specific
```

### 2.4 Missing Abstractions

| Function | Current State | Needed Abstraction |
|----------|--------------|-------------------|
| `I_GetFreeMem()` | Linux-only (`/proc/meminfo`) | Cross-platform API (sysctl, GlobalMemoryStatusEx) |
| `I_GetDiskFreeSpace()` | Multiple #ifdef blocks | Unified cross-platform API |
| `I_GetUserName()` | Windows + POSIX | Platform-agnostic |
| `I_mkdir()` | Windows vs POSIX differences | Cross-platform wrapper |
| `I_RegisterSong()` | Different signature on macOS | Unified API |

---

## 3. Recommended Solutions

### 3.1 CMake Platform Detection Improvements

Replace hardcoded definitions with CMake's platform detection:

```cmake
# Use CMAKE_SYSTEM to detect platform
# Use CMAKE_CXX_COMPILER_ID for compiler detection
# Remove hardcoded paths - use find_package() for all dependencies
```

### 3.2 Create Platform Abstraction Layer

Create new abstraction files:

- `include/platform/i_platform.h` - Core platform detection
- `interface/sdl/i_filesystem.cpp` - File system operations
- `interface/sdl/i_memory.cpp` - Memory info

### 3.3 Standardize Preprocessor Guards

Replace legacy defines with modern CMake-generated ones:

| Legacy Define | Modern Replacement |
|--------------|-------------------|
| `LINUX` | `CMAKE_SYSTEM_NAME STREQUAL "Linux"` |
| `WIN32` / `_WIN32` | `WIN32` (CMake sets this) |
| `__APPLE_CC__` | `CMAKE_SYSTEM_NAME STREQUAL "Darwin"` |
| `__linux__` | Platform check |

### 3.4 Use FindPKG Modules

Replace hardcoded paths with CMake's `find_package()`:

```cmake
# Instead of hardcoded paths:
find_package(TNL REQUIRED)
find_package(ENet 1.3 REQUIRED)
```

---

## 4. Phased Approach

### Phase 1: Build System Cleanup (Priority: HIGH)

**Goal:** Fix CMakeLists.txt to use proper package detection

**Tasks:**
1. Remove hardcoded `/home/geoffrey/tnl/*` paths - use CMake variables
2. Add proper FindTNL.cmake or use pkg-config
3. Standardize ENet detection (already mostly done)
4. Replace hardcoded `linux`, `i386`, `__GNUC__` with CMake-generated defines
5. Add proper platform detection for Windows, macOS

**Estimated files:** `CMakeLists.txt`

---

### Phase 2: Core Type Abstraction (Priority: HIGH)

**Goal:** Clean up `doomtype.h` platform detection

**Tasks:**
1. Replace `#ifdef __WIN32__` with standard `WIN32` check
2. Add macOS detection (`__APPLE__`)
3. Use `<cstdint>` instead of manual typedefs where possible
4. Remove legacy compiler checks (`__MSC__`, `__OS2__`, `__WATCOMC__`)
5. Standardize `O_BINARY` handling

**Estimated files:** `include/core/doomtype.h`

---

### Phase 3: System Interface Abstraction (Priority: HIGH)

**Goal:** Make `i_system.cpp` platform-agnostic

**Tasks:**
1. Replace `#ifdef LINUX` with CMake platform detection
2. Abstract `/dev/*` device paths (either detect at runtime or remove)
3. Make `I_GetFreeMem()` cross-platform using:
   - Windows: `GlobalMemoryStatusEx()`
   - macOS: `sysctl()`
   - Linux: Already works (keep but wrap)
4. Make `I_GetDiskFreeSpace()` cross-platform
5. Fix `I_GetUserName()` to use standard APIs
6. Fix `I_mkdir()` to use standard mkdir across platforms

**Estimated files:** `interface/sdl/i_system.cpp`, `include/i_system.h`

---

### Phase 4: Sound Interface Fixes (Priority: MEDIUM)

**Goal:** Fix macOS-specific function signature issue

**Tasks:**
1. Remove `#ifdef __MACOS__` from `I_RegisterSong()`
2. Use consistent function signature across platforms
3. Implement macOS-specific handling inside the function if needed

**Estimated files:** `include/i_sound.h`, `interface/sdl/i_sound.cpp`

---

### Phase 5: DLL Loading Abstraction (Priority: MEDIUM)

**Goal:** Clean up DLL loading code

**Tasks:**
1. Move platform-specific DLL code to implementation
2. Keep abstraction in header (already done)
3. Add macOS support if needed (currently only Windows + POSIX)

**Estimated files:** `include/m_dll.h`, `util/m_dll.cpp`

---

### Phase 6: Remaining Platform Code (Priority: LOW)

**Goal:** Clean up remaining platform-specific code

**Tasks:**
1. Fix `video/hwr_geometry.cpp` Windows code
2. Fix `engine/menu/menu.cpp` Windows code
3. Fix `engine/singleplayer_stubs.cpp` Windows stubs
4. Fix `util/console.cpp` non-SDL code
5. Remove legacy OS/2 support code

**Estimated files:** Various

---

## 5. Implementation Notes

### 5.1 Preprocessor Macro Strategy

Use CMake to generate a single config header:

```cmake
# CMakeLists.txt
configure_file(
    ${CMAKE_SOURCE_DIR}/include/config.h.in
    ${CMAKE_BINARY_DIR}/include/config.h
)
```

```c
// include/config.h.in
#cmakedefine HAVE_STDINT_H
#cmakedefine HAVE_INTTYPES_H
#cmakedefine PLATFORM_WINDOWS
#cmakedefine PLATFORM_LINUX
#cmakedefine PLATFORM_MACOS
#cmakedefine PLATFORM_BSD
```

### 5.2 Cross-Platform APIs

| Feature | Windows | Linux | macOS |
|---------|---------|-------|-------|
| Memory Info | `GlobalMemoryStatusEx()` | `/proc/meminfo` | `sysctl()` |
| Disk Space | `GetDiskFreeSpaceEx()` | `statfs()` | `statfs()` |
| Username | `GetUserName()` | `getpwuid()` | `getpwuid()` |
| DLL Loading | `LoadLibrary()` | `dlopen()` | `dlopen()` |
| Threads | `CreateThread()` | `pthread_create()` | `pthread_create()` |

### 5.3 Testing Strategy

After implementing changes:
1. Build on Linux (primary target)
2. Test build on Windows (MinGW/MSVC)
3. Test build on macOS
4. Verify runtime on all platforms

---

## 6. Summary

| Phase | Priority | Focus Area | Estimated Files |
|-------|----------|------------|-----------------|
| 1 | HIGH | Build system | 1 |
| 2 | HIGH | Core types | 1 |
| 3 | HIGH | System interface | 2-3 |
| 4 | MEDIUM | Sound interface | 2 |
| 5 | MEDIUM | DLL loading | 2 |
| 6 | LOW | Misc fixes | 5-6 |

The plan prioritizes fixing the build system first (Phase 1), as it currently has hardcoded paths that prevent building on non-Linux systems. Once the build system is fixed, the remaining phases can be addressed incrementally.

---

*Document created: 2026-02-21*
