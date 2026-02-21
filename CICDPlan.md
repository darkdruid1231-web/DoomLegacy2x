# CI/CD Implementation Plan for Doom Legacy

## Overview

This document outlines the implementation plan for adding Continuous Integration and Continuous Deployment (CI/CD) to Doom Legacy using GitHub Actions.

---

## 1. Current Build/Test Situation

### Build System
- **Primary**: CMake (minimum version 2.8.0)
- **Secondary**: Makefile (legacy, parallel to CMake)
- **Build type**: Out-of-tree builds required (CMake enforces this)
- **Output**: Single executable `doomlegacy` plus utility tools (`d2h`, `wadtool`)

### Dependencies Required
| Dependency | Purpose | Required |
|------------|---------|----------|
| SDL | Video/input abstraction | Yes |
| SDL_mixer | Audio/sound | Yes (when USE_SOUND=ON) |
| OpenGL | Graphics rendering | Yes |
| PNG | Image loading | Yes |
| JPEG | Image loading | Yes |
| ENet | Modern networking (1.3+) | Yes (when USE_MODERN_NETWORKING=ON) |
| TNL + TomCrypt | Legacy networking | Yes (when USE_MODERN_NETWORKING=OFF) |
| flex | Lexer generator | Yes |
| lemon | Parser generator | Yes |

### Current Test Infrastructure
- **Framework**: CTest (CMake test infrastructure)
- **Test types**: Unit tests + Integration tests
- **Test location**: `tests/` directory with `unit/` and `integration/` subdirectories
- **Test executables** (built via CMake):
  - `test_fixed_t` - Fixed-point arithmetic
  - `test_actor` - Actor/mobj system
  - `test_console` - Console output
  - `test_vector` - Vector math
  - `test_tnl_packets` - Network packets
  - `test_save_load_unit` - Save/load serialization
  - `test_serialization` - Data serialization
  - `test_demos_integration` - Demo recording
  - `test_network_integration` - Network code
  - `test_parity` - C++ vs C feature parity

### Build Commands
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
ctest  # Run tests
```

---

## 2. GitHub Actions Workflow Structure

### Workflow Files Location
```
.github/workflows/
├── build.yml          # Main build workflow
├── test.yml           # Test execution workflow  
└── release.yml        # Release/artifacts workflow
```

### Main Workflow: `build.yml`
- Triggers on: push to `main`, pull requests, and scheduled runs
- Uses matrix strategy for cross-platform builds
- Runs in parallel across matrix combinations

### Workflow Design Principles
1. **Fail-fast on critical issues**: Build failures block merges
2. **Cache dependencies**: Speed up subsequent builds
3. **Clear artifact names**: Easy identification of builds
4. **Conditional execution**: Skip unnecessary jobs where appropriate

---

## 3. Matrix Strategy for Multiple Platforms

### Platform Matrix
| OS | Compiler | CMake Version | Configurations |
|----|----------|---------------|----------------|
| Ubuntu Latest | GCC, Clang | 3.16, 3.22 | Debug, Release |
| Windows Latest | MSVC, MinGW | 3.22 | Release |
| macOS Latest | Apple Clang | 3.22 | Release |

### Detailed Matrix
```yaml
strategy:
  matrix:
    os: [ubuntu-latest, windows-latest, macos-latest]
    compiler: [gcc, clang]  # Linux only
    cmake: [3.16, 3.22]      # Linux only
    build_type: [Debug, Release]
    exclude:
      - os: windows-latest
        compiler: clang
      - os: macos-latest
        compiler: gcc
      # Limit combinations for practicality
      - os: ubuntu-latest
        cmake: 3.16
        build_type: Debug
```

### Platform-Specific Considerations

#### Linux (Ubuntu)
- Use `apt` for dependency installation
- Support both GCC and Clang
- Test multiple CMake versions
- Both Debug and Release builds

#### Windows
- Use MSVC or MinGW via Chocolatey
- ENet requires `ws2_32` and `winmm` linking
- Only Release builds (faster, more practical)

#### macOS
- Use Homebrew for dependencies
- Apple Clang only
- Only Release builds

---

## 4. Build Matrix (Compilers, CMake Versions)

### Compiler Versions
| Platform | Compiler | Version |
|----------|----------|---------|
| Linux | GCC | 11, 12, 13 |
| Linux | Clang | 14, 15, 16 |
| Windows | MSVC | Latest (visualstudio-latest) |
| Windows | MinGW | Latest |
| macOS | Apple Clang | 15 |

### CMake Versions
| Version | Purpose |
|---------|---------|
| 3.16 | Minimum supported (matches project requirements) |
| 3.22 | Modern features, better caching |
| 3.28 | Latest stable |

### Build Configurations
| Type | Flags | Purpose |
|------|-------|---------|
| Debug | `-DCMAKE_BUILD_TYPE=Debug` | Symbols, assertions enabled |
| Release | `-DCMAKE_BUILD_TYPE=Release` | Optimizations, no debug |

### Dependency Installation Strategy

#### Linux (Ubuntu)
```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    g++ \
    clang \
    libsdl2-dev \
    libsdl2-mixer-dev \
    libpng-dev \
    libjpeg-dev \
    libenet-dev \
    flex \
    lemon
```

#### Windows (MSVC)
```powershell
choco install cmake sdl2 sdl2-mixer libpng libjpeg enet flex leemon
```

#### macOS
```bash
brew install cmake sdl2 sdl2_mixer libpng libjpeg enet flex lemon
```

---

## 5. Test Execution Strategy

### Test Running
- Use CTest (`ctest`) for unified test execution
- Run with verbose output for debugging
- Enable test coloring for readability

### CTest Configuration
```bash
ctest --output-on-failure --verbose
```

### Test Categories

#### Quick Tests (every build)
- `test_fixed_t` - Core arithmetic (fast, fundamental)
- `test_vector` - Math library
- `test_console` - Utility tests

#### Integration Tests (full CI)
- `test_save_load_unit` - Savegame compatibility
- `test_serialization` - Network serialization
- `test_demos_integration` - Demo format
- `test_network_integration` - Network code

#### Network Tests (optional, requires setup)
- `test_tnl_packets` - TNL packet handling
- `test_parity` - C/C++ parity (may require TNL headers)

### Test Dependencies
- Some tests may require WAD files (documented in tests/README.md)
- SDL dependency for all tests (mock or real)

### Parallel Test Execution
```bash
ctest --parallel 4  # Run up to 4 tests concurrently
```

---

## 6. Artifact Publishing

### Build Artifacts
| Artifact | Contents | Platform |
|----------|----------|----------|
| `doomlegacy-{os}-{compiler}` | Binary executable | All |
| `doomlegacy-{os}-{compiler}.zip` | Zipped binary + deps | Windows |
| `doomlegacy-{os}-{compiler}.tar.gz` | Tarball for Linux/macOS | Unix |

### Artifact Retention
- Keep last 30 days of artifacts
- Maximum 10 artifacts per workflow run

### Artifact Upload Steps
```yaml
- name: Upload artifact
  uses: actions/upload-artifact@v4
  with:
    name: doomlegacy-${{ matrix.os }}-${{ matrix.compiler }}
    path: build/doomlegacy
    retention-days: 30
```

### Release Artifacts
- Create release on tag (e.g., `v1.9.9`)
- Upload platform-specific binaries
- Generate checksums (SHA256)

---

## 7. Step-by-Step Implementation

### Phase 1: Basic Build Pipeline

#### Step 1.1: Create GitHub Actions Directory
```bash
mkdir -p .github/workflows
```

#### Step 1.2: Create Basic Build Workflow
Create `.github/workflows/build.yml` with:
- Ubuntu build with GCC
- Dependency installation
- CMake configure + build
- Basic test execution

#### Step 1.3: Verify Basic Pipeline
- Push to test branch
- Check build succeeds
- Fix any issues

### Phase 2: Cross-Platform Support

#### Step 2.1: Add Windows Matrix
- Add Windows MSVC build
- Install dependencies via Chocolatey
- Test build

#### Step 2.2: Add macOS Matrix
- Add macOS build
- Install dependencies via Homebrew
- Test build

#### Step 2.3: Add Compiler Variants
- Add Clang on Linux
- Add MinGW on Windows
- Test compatibility

### Phase 3: Enhanced Testing

#### Step 3.1: Full Test Suite
- Enable all CTest tests
- Add test output capture
- Enable parallel testing

#### Step 3.2: Test Coverage (Optional)
- Add code coverage reporting
- Use `gcov` (GCC) or `llvm-cov` (Clang)
- Upload to Codecov or similar

#### Step 3.3: Integration Test Improvements
- Handle WAD file dependencies gracefully
- Skip tests that require game files
- Add smoke test script

### Phase 4: Artifacts and Releases

#### Step 4.1: Artifact Upload
- Configure artifact upload for each matrix job
- Use meaningful names
- Set retention policy

#### Step 4.2: Release Workflow
- Create `.github/workflows/release.yml`
- Trigger on version tags
- Upload release binaries
- Generate checksums

### Phase 5: Optimization

#### Step 5.1: Caching
- Add dependency caching (APT, Chocolatey, Homebrew)
- Add CMake build cache
- Cache vcpkg/Conan packages if used

#### Step 5.2: Performance Tuning
- Optimize matrix (reduce redundant combinations)
- Add conditional job execution
- Use build matrices efficiently

---

## Example Workflow File

### `.github/workflows/build.yml`
```yaml
name: Build and Test

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]
  schedule:
    - cron: '0 3 * * *'  # Nightly build

jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        compiler: [gcc, clang]
        cmake: [3.22]
        build_type: [Release]
        include:
          - os: windows-latest
            compiler: msvc
            cmake: 3.22
            build_type: Release

    steps:
    - uses: actions/checkout@v4

    - name: Install dependencies (Ubuntu)
      if: runner.os == 'Linux'
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake libsdl2-dev libsdl2-mixer-dev libpng-dev libjpeg-dev libenet-dev flex lemon

    - name: Install dependencies (Windows)
      if: runner.os == 'Windows'
      run: choco install cmake sdl2 sdl2-mixer libpng libjpeg enet flex lemon

    - name: Install dependencies (macOS)
      if: runner.os == 'macOS'
      run: brew install cmake sdl2 sdl2_mixer libpng libjpeg enet flex lemon

    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}

    - name: Build
      run: cmake --build build --parallel

    - name: Run Tests
      working-directory: build
      run: ctest --output-on-failure --verbose

    - name: Upload Artifact
      uses: actions/upload-artifact@v4
      with:
        name: doomlegacy-${{ matrix.os }}-${{ matrix.compiler }}
        path: build/doomlegacy
```

---

## Summary

This CI/CD implementation will provide:

1. **Automated builds** on multiple platforms (Linux, Windows, macOS)
2. **Compiler diversity** (GCC, Clang, MSVC)
3. **Comprehensive testing** via CTest
4. **Artifact management** for easy access to binaries
5. **Release automation** for tagged versions
6. **Fast builds** through caching and optimized matrices

The phased approach allows for incremental implementation, starting with basic builds and progressively adding complexity as the pipeline matures.

---

*Document Version: 1.0*
*Last Updated: 2026-02-21*
