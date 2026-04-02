//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// Test program for Modern C++ features verification
// Tests to ensure modern C++ patterns work correctly and provide
// the expected safety benefits (RAII, smart pointers, nullptr, constexpr).
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <algorithm>

#include "doomtype.h"
#include "m_fixed.h"
#include "qmus2mid.h"

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name)                                                                                 \
    do                                                                                             \
    {                                                                                              \
        tests_run++;                                                                               \
        cout << "  " << name << " ... ";                                                           \
    } while (0)

#define PASS()                                                                                     \
    do                                                                                             \
    {                                                                                              \
        tests_passed++;                                                                            \
        cout << "PASS" << endl;                                                                    \
    } while (0)

#define FAIL(msg)                                                                                  \
    do                                                                                             \
    {                                                                                              \
        tests_failed++;                                                                            \
        cout << "FAIL: " << msg << endl;                                                           \
    } while (0)

#define CHECK(cond, msg)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            FAIL(msg);                                                                             \
            return;                                                                                \
        }                                                                                         \
    } while (0)

//============================================================================
// constexpr Tests
//============================================================================

void test_constexpr_maxint()
{
    TEST("constexpr MAXINT_CXX");
    
    // Verify constexpr values match the macro definitions
    // Note: Some macros use casts (e.g., (short)0x8000), so we compare via static_cast
    CHECK(DoomLegacy::MAXINT_CXX == static_cast<int>(MAXINT), "MAXINT_CXX should equal MAXINT");
    CHECK(DoomLegacy::MININT_CXX == static_cast<int>(MININT), "MININT_CXX should equal MININT");
    CHECK(DoomLegacy::MAXSHORT_CXX == static_cast<int>(MAXSHORT), "MAXSHORT_CXX should equal MAXSHORT");
    CHECK(DoomLegacy::MINSHORT_CXX == static_cast<int>(MINSHORT), "MINSHORT_CXX should equal MINSHORT");
    CHECK(DoomLegacy::MAXCHAR_CXX == static_cast<int>(MAXCHAR), "MAXCHAR_CXX should equal MAXCHAR");
    CHECK(DoomLegacy::MINCHAR_CXX == static_cast<int>(MINCHAR), "MINCHAR_CXX should equal MINCHAR");
    
    PASS();
}

void test_constexpr_fixed_t()
{
    TEST("constexpr fixed_t constants");
    
    // Verify constexpr static members match enum values
    CHECK(fixed_t::FRACBITS == fixed_t::FBITS, "FRACBITS should equal FBITS");
    CHECK(fixed_t::FRACUNIT == fixed_t::UNIT, "FRACUNIT should equal UNIT");
    CHECK(fixed_t::FRACMIN == fixed_t::FMIN, "FRACMIN should equal FMIN");
    CHECK(fixed_t::FRACMAX == fixed_t::FMAX, "FRACMAX should equal FMAX");
    CHECK(fixed_t::FRACMASK == fixed_t::FMASK, "FRACMASK should equal FMASK");
    
    PASS();
}

//============================================================================
// nullptr Tests
//============================================================================

void test_nullptr()
{
    TEST("nullptr usage");
    
    // Test that nullptr works correctly
    int* int_ptr = nullptr;
    CHECK(int_ptr == nullptr, "nullptr should equal nullptr");
    CHECK(int_ptr == NULL, "nullptr should equal NULL (for backward compat)");
    
    // Test that we can assign nullptr to pointers
    const char* str_ptr = nullptr;
    CHECK(str_ptr == nullptr, "const char* nullptr should work");
    
    PASS();
}

//============================================================================
// Smart Pointer Tests (RAII)
//============================================================================

void test_unique_ptr_basic()
{
    TEST("std::unique_ptr basic functionality");
    
    // Test basic unique_ptr functionality
    std::unique_ptr<int> ptr(new int(42));
    CHECK(ptr != nullptr, "unique_ptr should not be null after construction");
    CHECK(*ptr == 42, "unique_ptr should hold correct value");
    
    // Test reset
    ptr.reset(new int(100));
    CHECK(*ptr == 100, "unique_ptr should hold new value after reset");
    
    // Test release
    int* raw = ptr.release();
    CHECK(ptr == nullptr, "unique_ptr should be null after release");
    CHECK(*raw == 100, "released pointer should hold correct value");
    delete raw;
    
    PASS();
}

void test_unique_ptr_array()
{
    TEST("std::unique_ptr array functionality");
    
    // Test unique_ptr array - simulating Track::data
    std::unique_ptr<char[]> buffer(new char[256]);
    CHECK(buffer != nullptr, "unique_ptr array should not be null");
    
    // Write some data
    buffer[0] = 'H';
    buffer[1] = 'i';
    buffer[2] = '\0';
    
    CHECK(buffer[0] == 'H', "unique_ptr array should allow write access");
    CHECK(buffer[1] == 'i', "unique_ptr array should allow read access");
    
    // Buffer automatically freed when going out of scope
    PASS();
}

void test_track_struct_raii()
{
    TEST("Track struct RAII (qmus2mid.h)");
    
    // Test that Track struct with smart pointer works
    Track track;
    
    // Initially should be null
    CHECK(track.data == nullptr, "Track data should initially be nullptr");
    
    // Allocate using reset (simulating qmus2mid.cpp)
    track.data.reset(new char[1024]);
    CHECK(track.data != nullptr, "Track data should not be null after allocation");
    
    // Write some data
    track.data[0] = 'T';
    track.data[1] = 'e';
    track.data[2] = 's';
    track.data[3] = 't';
    CHECK(track.data[0] == 'T', "Track data should be writable");
    
    // Reset (simulating FreeTracks)
    track.data.reset();
    CHECK(track.data == nullptr, "Track data should be nullptr after reset");
    
    PASS();
}

//============================================================================
// Lambda Tests (C++11)
//============================================================================

void test_lambda_basic()
{
    TEST("C++11 lambda basic functionality");
    
    // Test basic lambda
    auto add = [](int a, int b) { return a + b; };
    CHECK(add(2, 3) == 5, "Lambda should correctly add values");
    
    // Test lambda with capture
    int multiplier = 3;
    auto multiply = [multiplier](int x) { return x * multiplier; };
    CHECK(multiply(7) == 21, "Lambda with capture should work");
    
    // Test lambda with mutable capture
    int counter = 0;
    auto increment = [&counter]() { counter++; };
    increment();
    increment();
    CHECK(counter == 2, "Lambda with reference capture should modify variable");
    
    PASS();
}

void test_lambda_callback_pattern()
{
    TEST("C++11 lambda as callback");
    
    // Test lambda as callback (simulating action function usage)
    bool callback_executed = false;
    
    auto action_callback = [&callback_executed](int* actor) {
        callback_executed = true;
    };
    
    int dummy_actor = 0;
    action_callback(&dummy_actor);
    
    CHECK(callback_executed, "Lambda callback should have been executed");
    
    PASS();
}

void test_lambda_with_stl()
{
    TEST("C++11 lambda with STL algorithms");
    
    // Test lambda with STL containers
    std::string test_str = "Hello World";
    
    // Test using std::find with lambda
    const char target = 'W';
    auto found = std::find(test_str.begin(), test_str.end(), target);
    CHECK(found != test_str.end(), "std::find with lambda should find character");
    
    // Test using std::count_if with lambda
    auto is_upper = [](char c) { return c >= 'A' && c <= 'Z'; };
    int uppercase_count = std::count_if(test_str.begin(), test_str.end(), is_upper);
    CHECK(uppercase_count == 2, "Should find 2 uppercase letters in 'Hello World'");
    
    PASS();
}

//============================================================================
// std::function Tests
//============================================================================

void test_std_function()
{
    TEST("std::function wrapper");
    
    // Test std::function as type-erased callback
    std::function<int(int, int)> add_func = [](int a, int b) { return a + b; };
    CHECK(add_func(5, 7) == 12, "std::function should invoke lambda");
    
    // Test assigning different callable to std::function
    std::function<int(int, int)> multiply_func = [](int a, int b) { return a * b; };
    CHECK(multiply_func(6, 7) == 42, "std::function should work with different lambdas");
    
    PASS();
}

//============================================================================
// Main
//============================================================================

int main(int argc, char** argv)
{
    cout << "=== Modern C++ Feature Tests ===" << endl;
    cout << endl;
    
    cout << "--- constexpr Tests ---" << endl;
    test_constexpr_maxint();
    test_constexpr_fixed_t();
    cout << endl;
    
    cout << "--- nullptr Tests ---" << endl;
    test_nullptr();
    cout << endl;
    
    cout << "--- Smart Pointer / RAII Tests ---" << endl;
    test_unique_ptr_basic();
    test_unique_ptr_array();
    test_track_struct_raii();
    cout << endl;
    
    cout << "--- Lambda Tests ---" << endl;
    test_lambda_basic();
    test_lambda_callback_pattern();
    test_lambda_with_stl();
    cout << endl;
    
    cout << "--- std::function Tests ---" << endl;
    test_std_function();
    cout << endl;
    
    cout << "=== Summary ===" << endl;
    cout << "Tests run:    " << tests_run << endl;
    cout << "Tests passed: " << tests_passed << endl;
    cout << "Tests failed: " << tests_failed << endl;
    
    if (tests_failed > 0)
    {
        cout << endl << "SOME TESTS FAILED!" << endl;
        return 1;
    }
    
    cout << endl << "All tests PASSED!" << endl;
    return 0;
}
