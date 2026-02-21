# CMake Upgrade Plan: Doom Legacy 2.8.0 → 3.16+

## Overview

This document outlines the migration plan for upgrading CMake from version 2.8.0 to 3.16+ in the Doom Legacy codebase. The upgrade enables modern CMake features, better compiler detection, improved build performance, and future-proofing.

---

## 1. Current Issues with CMake 2.8.0

### 1.1 Deprecated/Legacy Patterns

| Issue | Current Code | Impact |
|-------|---------------|--------|
| **Old-style include directories** | `include_directories(${SDL_INCLUDE_DIR})` | Global includes affect all targets; deprecated since CMake 3.0 |
| **Old-style definitions** | `add_definitions("-DUSE_SOUND")` | Global definitions; deprecated since CMake 3.0 |
| **Old-style linking** | `${SDL_LIBRARY}` in `target_link_libraries` | Legacy linking style without modern target-based approach |
| **Deprecated compiler check** | `CMAKE_COMPILER_IS_GNUCXX` | Removed in CMake 3.6+ |
| **Outdated C++ standard** | `-std=c++0x` | C++0x was replaced by C++11 in 2011 |
| **Old-style FindFlex** | `find_package(FLEX)` | Old FindFlex module |
| **Missing policy defaults** | No `cmake_policy()` calls | Relies on implicit policy behavior |

### 1.2 Missing Modern Features

- No modern target-based build system usage
- No generator expressions for configuration-dependent paths
- No `target_compile_options()` for per-target compiler flags
- No `target_include_directories()` for proper include management
- No modern find modules (uses outdated ones)
- No support for Unity Build
- No precompiled header support

### 1.3 Portability Issues

- Hardcoded library output paths
- Platform-specific code using outdated checks
- No proper RPATH management
- Windows-specific linking via global variables

---

## 2. Breaking Changes Between CMake 2.8 and 3.16

### 2.1 Major Policy Changes

| Policy | Version | Change |
|--------|---------|--------|
| CMP0025 | 3.0 | Apple Clang is now identified as "AppleClang" (not "Clang") |
| CMP0026 | 3.0 | `CMAKE_LANG_COMPILER_ID` access restricted |
| CMP0038 | 3.0 | Targets with custom commands must have explicit output |
| CMP0042 | 3.0 | MACOSX_RPATH enabled by default |
| CMP0045 | 3.0 | `get_target_property()` fails on non-existent targets |
| CMP0048 | 3.0 | `project()` sets `PROJECT_VERSION` variables |
| CMP0054 | 3.0 | Only interpret `if()` arguments as variables when unquoted |
| CMP0057 | 3.2 | `in` operator in `if()` requires CMake 3.3+ |
| CMP0063 | 3.3 | C++ visibility settings for hidden symbol visibility |
| CMP0067 | 3.16 | `CMAKE_*_COMPILER_ID` must be used instead of `CMAKE_COMPILER_IS_*` |
| CMP0097 | 3.16 | `GIT_SUBMODBERS` "" now initializes no submodules |

### 2.2 Deprecated/Removed Features

| Feature | Status | Replacement |
|---------|--------|-------------|
| `CMAKE_COMPILER_IS_GNUCXX` | **Removed (3.6+)** | `CMAKE_CXX_COMPILER_ID` + `CMAKE_CXX_COMPILER_VERSION` |
| `CMAKE_COMPILER_IS_GNUCC` | **Removed (3.6+)** | `CMAKE_C_COMPILER_ID` |
| `add_definitions()` | Deprecated | `target_compile_definitions()` |
| `include_directories()` | Deprecated | `target_include_directories()` |
| `link_directories()` | Deprecated | `target_link_directories()` |
| `add_definitions(-DFOO)` | Deprecated | `target_compile_definitions(target FOO)` |
| `LINK_LIBRARIES` | Deprecated | Modern `target_link_libraries()` |
| FindModule deprecated vars | Various | Use imported targets |
| `-std=c++0x` | Deprecated | `-std=c++11` or later |

### 2.3 New Features Available in 3.16+

- **Modern target-based build system**
- **Generator expressions** (`$<...>`)
- **Precompiled headers** (`target_precompile_headers()`)
- **Unity build** (`UNITY_BUILD` target property)
- **Improved find_package()** with `CONFIG` mode
- **CTest resource allocation**
- **Better Windows support** (export symbols automatically)

---

## 3. Step-by-Step Migration Plan

### Phase 1: Preparation (Non-breaking)

#### Step 1.1: Update Minimum Version
```cmake
# Before
cmake_minimum_required(VERSION 2.8.0)

# After
cmake_minimum_required(VERSION 3.16)
project(doomlegacy VERSION 1.9.9)
```

**Effort:** Low (1 file, single line change)  
**Risk:** Low  
**Testing:** Verify CMake 3.16+ is available on build systems

---

#### Step 1.2: Replace Compiler Detection
```cmake
# Before
if(CMAKE_COMPILER_IS_GNUCXX)
    set(c0flag "-std=c++0x -fpermissive -w")
    CHECK_CXX_COMPILER_FLAG(${c0flag} HAS_C0X)
    if(NOT ${HAS_C0X})
        message(FATAL_ERROR "Your compiler does not support c++0x features. Can not build.")
    endif()
    add_definitions(${c0flag})
else()
    message(FATAL_ERROR "Only GNU C++ is supported at the moment.")
endif()

# After
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Check for C++11 support
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(NOT COMPILER_SUPPORTS_CXX11)
        message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER_ID} has no C++11 support.")
    endif()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
else()
    message(FATAL_ERROR "Only GNU C++ and Clang are supported at the moment.")
endif()
```

**Effort:** Medium (requires restructuring compiler check logic)  
**Risk:** Medium  
**Testing:** Verify compilation on supported platforms

---

#### Step 1.3: Update Find Modules
```cmake
# Before
find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(FLEX REQUIRED)

# After - Use modern imported targets where available
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Flex REQUIRED)  # Case-sensitive in CMake 3+
```

**Effort:** Medium (depends on library availability)  
**Risk:** Medium (may need to adjust library names)  
**Testing:** Verify all packages are found

---

### Phase 2: Modernize Target Structure

#### Step 2.1: Replace include_directories() with target_include_directories()

**File: CMakeLists.txt (root)**
```cmake
# Before
include_directories(${SDL_INCLUDE_DIR} ${SDL_MIXER_INCLUDE_DIR})
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(include)
include_directories(tnl)

# After - Add to root CMakeLists.txt
add_library(util STATIC ...)  # Define util first

add_executable(doomlegacy ${LEGACY_SRC})

target_include_directories(doomlegacy PRIVATE
    ${SDL_INCLUDE_DIR}
    ${SDL_MIXER_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tnl
)

# For static library grammars
target_include_directories(grammars PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# For util library
target_include_directories(util PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
```

**Effort:** High (requires modifying all CMakeLists.txt files)  
**Risk:** Medium  
**Testing:** Verify all includes are correct after build

---

#### Step 2.2: Replace add_definitions() with target_compile_definitions()

```cmake
# Before
add_definitions("-DUSE_SOUND")
add_definitions("-DSDL")
add_definitions("-DFN_CDECL=")
add_definitions("-Di386")
add_definitions("-Dlinux")
add_definitions("-D__GNUC__")
add_definitions("-DQSORT_CALLBACK=")

# After - Apply to specific targets
target_compile_definitions(doomlegacy PRIVATE
    USE_SOUND
    SDL
    FN_CDECL=
    i386
    linux
    __GNUC__
    QSORT_CALLBACK=
)
```

**Effort:** Medium  
**Risk:** Low  
**Testing:** Build succeeds with same defines

---

#### Step 2.3: Modernize Library Finding (use imported targets)

```cmake
# Before
find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
target_link_libraries(doomlegacy ${SDL_LIBRARY})

# After - Use modern approach
find_package(SDL2 REQUIRED)
# Link using modern imported targets
target_link_libraries(doomlegacy PRIVATE
    SDL2::SDL2
    SDL2::SDL2main  # On Windows
)
```

**Note:** SDL 1.2 uses old-style FindSDL, SDL2 uses modern FindSDL2 with imported targets.

**Effort:** Medium  
**Risk:** Medium (depends on package availability)  
**Testing:** Verify linking works correctly

---

### Phase 3: Advanced Improvements

#### Step 3.1: Add Precompiled Headers Support

```cmake
# Add to root CMakeLists.txt
target_precompile_headers(doomlegacy PRIVATE
    <vector>
    <string>
    <map>
    # Add commonly-used headers
)
```

**Effort:** Low  
**Risk:** Low  
**Testing:** Verify compilation time improvement

---

#### Step 3.2: Add Unity Build Support (Optional)

```cmake
# Add option
option(ENABLE_UNITY_BUILD "Enable unity build for faster compilation" OFF)

if(ENABLE_UNITY_BUILD)
    set_target_properties(doomlegacy PROPERTIES UNITY_BUILD ON)
endif()
```

**Effort:** Low  
**Risk:** Low  
**Testing:** Verify build produces identical output

---

#### Step 3.3: Improve RPATH Handling

```cmake
# Add for Linux/macOS builds
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
```

**Effort:** Low  
**Risk:** Low  
**Testing:** Verify executable runs from non-installed location

---

### Phase 4: Test Infrastructure Improvements

#### Step 4.1: Modernize Test Configuration

```cmake
# Improve test definitions with modern patterns
add_test(NAME doomlegacy.fixed_t COMMAND test_fixed_t)
add_test(NAME doomlegacy.serialization COMMAND test_serialization)
```

**Effort:** Low  
**Risk:** Low  
**Testing:** `ctest` runs all tests

---

## 4. Testing Strategy at Each Step

### Phase 1 Testing

| Step | Test Method | Success Criteria |
|------|-------------|------------------|
| 1.1 | Run `cmake -B build -S .` | CMake 3.16+ accepts configuration |
| 1.2 | Compile with GCC/Clang | No compiler detection errors |
| 1.3 | Run `find_package()` | All required packages found |

### Phase 2 Testing

| Step | Test Method | Success Criteria |
|------|-------------|------------------|
| 2.1 | Build all targets | No include path errors |
| 2.2 | Build all targets | No definition errors |
| 2.3 | Link and run | Executable launches |

### Phase 3 Testing

| Step | Test Method | Success Criteria |
|------|-------------|------------------|
| 3.1 | Build with PCH | Faster compilation |
| 3.2 | Build with Unity | Identical output |
| 3.3 | Run from build dir | No missing library errors |

### Phase 4 Testing

| Step | Test Method | Success Criteria |
|------|-------------|------------------|
| 4.1 | Run `ctest` | All tests pass |

### Full Integration Testing

```bash
# Clean build
rm -rf build
mkdir build && cd build

# Configure with CMake 3.16+
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure

# Run the game (if available)
./doomlegacy
```

---

## 5. Estimated Effort for Each Step

| Phase | Step | Description | Effort (hours) |
|-------|------|-------------|----------------|
| 1.1 | Update minimum version | Change version in 1 file | 0.25 |
| 1.2 | Replace compiler detection | Rewrite compiler check logic | 1.0 |
| 1.3 | Update Find modules | Update find_package calls | 1.0 |
| 2.1 | Replace include_directories | Modify all CMakeLists.txt | 2.0 |
| 2.2 | Replace add_definitions | Modify root CMakeLists.txt | 1.0 |
| 2.3 | Modernize library linking | Update link commands | 1.5 |
| 3.1 | Add PCH support | Add precompile_headers() | 0.5 |
| 3.2 | Unity build option | Add option + property | 0.5 |
| 3.3 | Improve RPATH | Add RPATH settings | 0.5 |
| 4.1 | Test infrastructure | Review and improve tests | 0.5 |
| - | **Buffer for issues** | - | 2.0 |
| **Total** | | | **~10.75 hours** |

### Effort Summary

- **Phase 1 (Preparation):** ~2.25 hours
- **Phase 2 (Modernize Targets):** ~4.5 hours  
- **Phase 3 (Advanced):** ~1.5 hours
- **Phase 4 (Testing):** ~0.5 hours
- **Buffer:** ~2.0 hours
- **Total:** ~10.75 hours

---

## 6. Risk Mitigation

### High-Risk Items

1. **Library availability** - Some packages may not have CMake config files
   - *Mitigation:* Use pkg-config fallback or create custom Find module

2. **C++ standard changes** - Moving from C++0x to C++11 may break code
   - *Mitigation:* Test thoroughly, may need to adjust code for stricter C++11

3. **Platform-specific code** - Windows linking may need adjustments
   - *Mitigation:* Test on all target platforms

### Rollback Plan

If issues arise, the project can be partially migrated:
1. Keep `cmake_minimum_required(VERSION 3.16)` for new features
2. Keep some deprecated patterns for compatibility
3. Use `cmake_policy(SET CMPxxxx OLD)` to suppress specific warnings

---

## 7. Post-Migration Benefits

After completing this upgrade, the project will have:

- ✅ Modern target-based build system
- ✅ Better compiler portability
- ✅ Improved compilation times (PCH, Unity Build)
- ✅ Proper RPATH handling
- ✅ Access to modern CMake features
- ✅ Better IDE integration
- ✅ Easier maintenance

---

## 8. References

- [CMake 3.16 Release Notes](https://cmake.org/cmake/help/v3.16/release/3.16/)
- [CMake Policies Manual](https://cmake.org/cmake/help/v3.16/manual/cmake-policies.7.html)
- [Modern CMake Best Practices](https://cmake.org/cmake/help/latest/manual/cmake-best-practices.7.html)
