# Test Framework Migration Plan

## Overview

This document outlines a plan to migrate Doom Legacy's test suite from the current custom test framework to a modern C++ testing framework (Google Test or Catch2).

---

## 1. Current Test Framework Analysis

### 1.1 Existing Test Infrastructure

The current test suite uses a **custom ad-hoc framework** with locally-defined macros in each test file:

```cpp
static int tests_run = 0;
static int tests_passed = 0;
static string last_failure;

#define TEST(name) do { ... } while(0)
#define PASS() do { ... } while(0)
#define FAIL(msg) do { ... } while(0)
#define CHECK(cond, msg) do { ... } while(0)
#define ASSERT_EQ(expected, actual, msg) do { ... } while(0)
#define ASSERT_NEAR(expected, actual, tolerance, msg) do { ... } while(0)
```

### 1.2 Test Files and Structure

| Test Executable | Source File | Type | Status |
|-----------------|-------------|------|--------|
| test_fixed_t | tests/unit/test_fixed_t.cpp | Unit | Active |
| test_actor | tests/unit/test_actor.cpp | Unit | Active |
| test_console | tests/unit/test_console.cpp | Unit | Active |
| test_vector | tests/unit/test_vector.cpp | Unit | Active |
| test_tnl_packets | tests/unit/test_tnl_packets.cpp | Unit | Active |
| test_serialization | tests/integration/test_serialization.cpp | Integration | Active |
| test_demos_integration | tests/integration/test_demos.cpp | Integration | Active |
| test_network_integration | tests/integration/test_network_integration.cpp | Integration | Active |
| test_parity | tests/integration/test_parity.cpp | Integration | Active |
| test_save_load_unit | tests/integration/test_save_load.cpp | Integration | Active |
| test_mapinfo_constants | tests/unit/test_mapinfo.cpp | Unit | (Commented out) |
| test_console_colors | — | — | (Not implemented) |

### 1.3 Current Build System

- **Build Tool**: CMake
- **Test Invocation**: Each test has its own `main()` function
- **Test Runner**: CTest (via `enable_testing()` and `add_test()`)
- **Return Code**: 0 = success, 1 = failure

### 1.4 Limitations of Current Framework

| Issue | Impact |
|-------|--------|
| No test discovery | Must manually list all tests in main() |
| No test filtering | Cannot run subsets of tests |
| No parameterized tests | Cannot easily test multiple inputs |
| No fixtures | No setup/teardown support |
| No XML/JUnit output | CI integration requires custom parsing |
| No assertions beyond basic checks | Limited expressive power |
| Duplicated macro definitions | Every file redefines the same macros |
| No thread-safety | Cannot run tests in parallel |
| No death tests | Cannot test crash scenarios |

---

## 2. Framework Options: Google Test vs Catch2

### 2.1 Comparison Matrix

| Feature | Google Test | Catch2 | Winner |
|---------|-------------|--------|--------|
| **License** | BSD 3-Clause | Boost 1.0 | Catch2 (more permissive) |
| **Header-only** | No (requires linking) | Yes | Catch2 |
| **C++ Version** | C++11 minimum | C++11 minimum | Tie |
| **Test Discovery** | Automatic via RUN_ALL_TESTS() | Automatic via CATCH_CONFIG_MAIN | Catch2 |
| **BDD-style** | Via GoogleMock | Native SCENARIO/GIVEN/WHEN/THEN | Catch2 |
| **Death Tests** | Yes | No | Google Test |
| **Typed Tests** | Yes | Via TEMPLATE_TEST_CASE | Google Test |
| **Value-Parameterized** | Yes | Via TEST_CASE_METHOD with generator | Google Test |
| **Floating-point Comparisons** | FloatLE/FloatEQ | Approx | Catch2 (cleaner API) |
| **XML Output** | Yes (XML report) | Yes (XML report) | Tie |
| **CI Integration** | Excellent | Excellent | Tie |
| **Learning Curve** | Moderate | Low | Catch2 |
| **Assertion Syntax** | ASSERT_* / EXPECT_* | REQUIRE / CHECK | Subjective |
| **Current Industry Usage** | Very High | High | Google Test |

### 2.2 Recommendation

**Recommended Framework: Catch2**

Reasons:
1. **Header-only** — Simplifies build configuration, no extra linking
2. **Easier migration** — Catch2's REQUIRE/CHECK reads similarly to custom macros
3. **Better for this codebase** — Simpler API matches the simple needs of the project
4. **Boost licensed** — More permissive than BSD for GPL projects
5. **Approx** — Cleaner floating-point assertions than Google Test

**Alternative**: Google Test if death tests or typed tests become essential.

---

## 3. Iterative Migration Strategy

### 3.1 Migration Phases

```
Phase 1: Preparation (Week 1)
├── Add Catch2 as dependency
├── Create common test utilities header
├── Update CMakeLists.txt for mixed framework
└── Verify build still works

Phase 2: Pilot Migration (Week 1-2)
├── Migrate test_fixed_t (smallest, most complete)
├── Verify tests pass in both frameworks
├── Document migration patterns
└── Create migration checklist

Phase 3: Unit Tests Migration (Week 2-3)
├── test_actor
├── test_console
├── test_vector
├── test_tnl_packets
└── test_mapinfo (if applicable)

Phase 4: Integration Tests Migration (Week 3-4)
├── test_serialization
├── test_demos_integration
├── test_network_integration
├── test_parity
└── test_save_load_unit

Phase 4b: New Test Files (as needed)
├── test_console_colors (if needed)
└── Any new tests

Phase 5: Cleanup (Week 4)
├── Remove custom macros from all files
├── Simplify CMakeLists.txt
├── Enable parallel test execution
└── Update documentation
```

### 3.2 Migration Order (Priority)

| Priority | Test File | Rationale |
|----------|-----------|-----------|
| 1 | test_fixed_t | Smallest, self-contained, good coverage |
| 2 | test_vector | Good mix of unit test patterns |
| 3 | test_actor | Tests constants/flags, minimal dependencies |
| 4 | test_console | Moderate complexity |
| 5 | test_tnl_packets | Network-related, moderate |
| 6 | test_serialization | Integration tests begin |
| 7 | test_save_load_unit | File I/O integration |
| 8 | test_parity | Legacy parity tests |
| 9 | test_demos_integration | Demo parsing |
| 10 | test_network_integration | Network integration |

### 3.3 Per-Test Migration Steps

For each test file:

1. **Add Catch2 include**: `#include <catch2/catch_all.hpp>`
2. **Replace main()**: Either use `CATCH_CONFIG_MAIN` or create custom main with Catch2
3. **Convert TEST()**: → `TEST_CASE("name")`
4. **Convert PASS()**: → Test passes automatically when it completes
5. **Convert FAIL()**: → `REQUIRE(false)` or `FAIL("message")`
6. **Convert CHECK()**: → `REQUIRE(condition)` or `CHECK(condition)`
7. **Convert ASSERT_EQ()**: → `REQUIRE(expected == actual)` or `INFO()` for better output
8. **Convert ASSERT_NEAR()**: → `REQUIRE(actual Approx(expected).margin(tolerance))`
9. **Remove custom macros**: Delete the `#define` section
10. **Update test name format**: Use descriptive names like "fixed_t arithmetic: addition works"

---

## 4. Running Tests During Migration

### 4.1 Mixed Framework Support

The migration allows running both old and new tests simultaneously:

```cmake
# CMakeLists.txt - Support both frameworks during migration

# Find Catch2
find_package(Catch2 QUIET)
if(Catch2_FOUND)
    message(STATUS "Catch2 found: ${Catch2_VERSION}")
    set(USE_CATCH2 ON)
else()
    message(STATUS "Catch2 not found, using legacy tests only")
    set(USE_CATCH2 OFF)
endif()

# Legacy tests (always available)
add_executable(test_fixed_t_legacy tests/unit/test_fixed_t.cpp)
add_test(NAME fixed_t_legacy COMMAND test_fixed_t_legacy)

# New Catch2 tests (if Catch2 available)
if(USE_CATCH2)
    add_executable(test_fixed_t tests/unit/test_fixed_t_new.cpp)
    target_link_libraries(test_fixed_t PRIVATE Catch2::Catch2WithMain)
    add_test(NAME fixed_t_catch2 COMMAND test_fixed_t)
    
    # Alternative: migrate file in-place
    add_executable(test_vector tests/unit/test_vector.cpp)
    target_link_libraries(test_vector PRIVATE Catch2::Catch2WithMain)
    add_test(NAME vector_catch2 COMMAND test_vector)
endif()
```

### 4.2 Running Tests

```bash
# Run all tests (both legacy and new)
ctest --output-on-failure

# Run only new Catch2 tests
ctest -R "_catch2" --output-on-failure

# Run only legacy tests
ctest -R "_legacy" --output-on-failure

# Run specific test
ctest -R "fixed_t" --output-on-failure

# Run with verbose output
ctest -V

# Run in parallel (after full migration)
ctest --parallel 4
```

### 4.3 Build Configuration

```bash
# With Catch2 (recommended)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure

# Without Catch2 (legacy only)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCATCH2_ROOT=/path/to/catch2
# or
cmake -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
```

---

## 5. CI/CD Integration

### 5.1 GitHub Actions Example

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake libsdl2-dev
          
      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release
        # Catch2 will be fetched automatically via FetchContent
        
      - name: Build
        run: cmake --build build -j$(nproc)
        
      - name: Run tests
        run: |
          ctest --test-dir build \
            --output-on-failure \
            --timeout 300
            
      - name: Upload test results
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: test-results
          path: build/Testing/**/*.xml
```

### 5.2 Catch2 XML Output

Enable XML output for CI integration:

```cmake
# In CMakeLists.txt - Add XML output support
# (Optional - Catch2 can generate XML via command line)
```

```bash
# Run tests with XML output
./test_fixed_t --reporter xml --out test-results.xml

# Or use CTest's built-in output capture
ctest --test-dir build -j4 --output-on-failure -D ExperimentalTest
```

### 5.3 Test Timeout Configuration

```cmake
# In CMakeLists.txt
add_test(NAME fixed_t_tests COMMAND test_fixed_t)
set_tests_properties(fixed_t_tests PROPERTIES TIMEOUT 60)
```

### 5.4 Parallel Test Execution

After full migration, enable parallel execution:

```bash
# Run up to 4 tests in parallel
ctest --parallel 4

# Or configure in CMakeLists.txt
ctest_configure(CTEST_PARALLEL_LEVEL 4)
```

---

## 6. Each Test Migration Checklist

### Template: Test File Migration

- [ ] **Analyze current test file**
  - [ ] List all test functions
  - [ ] Identify custom macros used
  - [ ] Note any fixtures/setup needed
  - [ ] Check dependencies

- [ ] **Create new Catch2 version**
  - [ ] Add `#include <catch2/catch_all.hpp>`
  - [ ] Add `#define CATCH_CONFIG_MAIN` (or custom main)
  - [ ] Convert each `TEST(name)` to `TEST_CASE("description")`
  - [ ] Convert `PASS()` (implicit in Catch2)
  - [ ] Convert `FAIL(msg)` to `FAIL(msg)` or `REQUIRE(false)`
  - [ ] Convert `CHECK(cond, msg)` to `REQUIRE(cond)` with message
  - [ ] Convert `ASSERT_EQ(a, b, msg)` to `REQUIRE(a == b)`
  - [ ] Convert `ASSERT_NEAR(a, b, tol)` to `REQUIRE(a == Approx(b).margin(tol))`

- [ ] **Build and test**
  - [ ] Add to CMakeLists.txt with Catch2 linkage
  - [ ] Compile successfully
  - [ ] All tests pass
  - [ ] Verify output is clear

- [ ] **Update integration**
  - [ ] Add to CTest
  - [ ] Verify `ctest` runs it
  - [ ] Update any CI configurations

- [ ] **Cleanup (post-migration)**
  - [ ] Remove old test file (or keep as backup)
  - [ ] Remove legacy CMake entry
  - [ ] Verify only Catch2 version remains

---

## 7. Implementation Notes

### 7.1 Catch2 Installation Options

**Option A: FetchContent (Recommended)**
```cmake
include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(catch2)
```

**Option B: System Package**
```bash
# Ubuntu/Debian
sudo apt install catch2

# macOS
brew install catch2
```

**Option C: vcpkg**
```bash
vcpkg install catch2
```

### 7.2 Migration Pattern Examples

**Before (Custom Framework):**
```cpp
void test_addition() {
    TEST("addition");
    fixed_t a(10), b(20);
    fixed_t c = a + b;
    CHECK(c.value() == (30 << fixed_t::FBITS), "addition failed");
    PASS();
}
```

**After (Catch2):**
```cpp
TEST_CASE("fixed_t addition", "[fixed_t][arithmetic]") {
    fixed_t a(10), b(20);
    fixed_t c = a + b;
    REQUIRE(c.value() == (30 << fixed_t::FBITS));
}
```

**With Better Messages:**
```cpp
TEST_CASE("fixed_t addition", "[fixed_t][arithmetic]") {
    fixed_t a(10), b(20);
    fixed_t c = a + b;
    
    INFO("Testing: 10 + 20 should equal 30");
    REQUIRE(c.value() == (30 << fixed_t::FBITS));
}
```

### 7.3 Tags for Test Organization

Use Catch2 tags for filtering:

```cpp
TEST_CASE("constructor from int", "[fixed_t][constructor][unit]") { ... }
TEST_CASE("addition", "[fixed_t][arithmetic][unit]") { ... }
TEST_CASE("save/load roundtrip", "[serialization][integration]") { ... }
```

Run specific tags:
```bash
./test_fixed_t "[fixed_t][constructor]"
./test_fixed_t "~[slow]"  # Exclude slow tests
```

---

## 8. Rollback Plan

If migration encounters issues:

1. **Keep both versions**: Legacy tests remain functional during migration
2. **Feature flags**: Use CMake `USE_CATCH2` option to switch frameworks
3. **Incremental**: Migrate one test at a time, keeping others working
4. **Backup**: Keep old test files with `_legacy` suffix until fully validated

---

## 9. Success Criteria

- [ ] All existing tests pass in new framework
- [ ] Build system detects and uses Catch2
- [ ] `ctest` runs all tests successfully
- [ ] CI/CD pipeline runs tests automatically
- [ ] Test output is clear and actionable
- [ ] No custom test macros remain
- [ ] Parallel test execution works

---

## 10. Timeline Estimate

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| Preparation | 1 day | Build system updated |
| Pilot Migration | 2 days | test_fixed_t migrated |
| Unit Tests | 3-4 days | All unit tests migrated |
| Integration Tests | 3-4 days | All integration tests migrated |
| Cleanup | 1 day | Final integration |

**Total: ~2 weeks for full migration**

---

*Document Version: 1.0*
*Created: 2026-02-21*
