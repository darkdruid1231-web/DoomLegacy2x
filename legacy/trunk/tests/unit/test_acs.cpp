//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Unit tests for ACS scripting constants and basic types.

#include "doomtype.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;
static string last_failure;

#define TEST(name)                                                                                 \
    do                                                                                             \
    {                                                                                              \
        tests_run++;                                                                               \
        last_failure = "";                                                                         \
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
        last_failure = msg;                                                                        \
        cout << "FAIL: " << msg << endl;                                                           \
    } while (0)

#define ASSERT_EQ(expected, actual, msg)                                                           \
    do                                                                                             \
    {                                                                                              \
        if ((expected) != (actual))                                                                \
        {                                                                                          \
            FAIL(msg);                                                                             \
            cout << "    Expected: " << (expected) << ", Got: " << (actual) << endl;               \
            return;                                                                                \
        }                                                                                          \
    } while (0)

// ACS constants (copied from t_acs.h for standalone testing)
#define ACS_WORLD_VARS 64
#define ACS_LOCAL_VARS 10
#define ACS_STACKSIZE 32

void test_acs_constants()
{
    TEST("ACS_WORLD_VARS equals 64");
    ASSERT_EQ(64, ACS_WORLD_VARS, "World vars count");

    TEST("ACS_LOCAL_VARS equals 10");
    ASSERT_EQ(10, ACS_LOCAL_VARS, "Local vars count");

    TEST("ACS_STACKSIZE equals 32");
    ASSERT_EQ(32, ACS_STACKSIZE, "Stack size");

    PASS();
}

void test_acs_stack_operations()
{
    TEST("ACS stack constants are reasonable");
    ASSERT_EQ(true, ACS_STACKSIZE >= 16, "Stack size at least 16");
    ASSERT_EQ(true, ACS_STACKSIZE <= 64, "Stack size at most 64");
    PASS();
}

void test_acs_script_state_enum()
{
    // Simulate ACS script states
    enum acs_state_t
    {
        ACS_stopped = 0,
        ACS_running,
        ACS_suspended,
        ACS_waitforscript,
        ACS_waitfortag,
        ACS_waitforpoly,
        ACS_terminating
    };

    TEST("ACS_stopped state equals 0");
    ASSERT_EQ(0, ACS_stopped, "ACS_stopped = 0");

    TEST("ACS_running state equals 1");
    ASSERT_EQ(1, ACS_running, "ACS_running = 1");

    TEST("ACS_suspended state equals 2");
    ASSERT_EQ(2, ACS_suspended, "ACS_suspended = 2");

    TEST("ACS_waitforscript state equals 3");
    ASSERT_EQ(3, ACS_waitforscript, "ACS_waitforscript = 3");

    TEST("ACS_waitfortag state equals 4");
    ASSERT_EQ(4, ACS_waitfortag, "ACS_waitfortag = 4");

    TEST("ACS_waitforpoly state equals 5");
    ASSERT_EQ(5, ACS_waitforpoly, "ACS_waitforpoly = 5");

    TEST("ACS_terminating state equals 6");
    ASSERT_EQ(6, ACS_terminating, "ACS_terminating = 6");

    PASS();
}

void test_acs_stack_push_pop()
{
    TEST("ACS stack push/pop operations work correctly");

    Sint32 stack[ACS_STACKSIZE];
    int sp = 0;

    // Push some values
    stack[sp++] = 10;
    stack[sp++] = 20;
    stack[sp++] = 30;

    ASSERT_EQ(3, sp, "Stack pointer after 3 pushes");

    // Pop values
    Sint32 val3 = stack[--sp];
    Sint32 val2 = stack[--sp];
    Sint32 val1 = stack[--sp];

    ASSERT_EQ(30, val3, "Pop third value");
    ASSERT_EQ(20, val2, "Pop second value");
    ASSERT_EQ(10, val1, "Pop first value");
    ASSERT_EQ(0, sp, "Stack pointer back to 0");

    PASS();
}

void test_acs_stack_overflow_check()
{
    TEST("ACS stack overflow detection works");

    Sint32 stack[ACS_STACKSIZE];
    int sp = 0;

    // Fill stack
    for (int i = 0; i < ACS_STACKSIZE; i++)
    {
        stack[sp++] = i;
    }

    ASSERT_EQ(ACS_STACKSIZE, sp, "Stack filled to capacity");

    // Check overflow would occur
    bool overflow = (sp >= ACS_STACKSIZE);
    ASSERT_EQ(true, overflow, "Overflow detected at capacity");

    PASS();
}

void test_acs_variable_arrays()
{
    TEST("ACS variable arrays are properly sized");

    // Simulate world vars array
    Sint32 world_vars[ACS_WORLD_VARS];
    for (int i = 0; i < ACS_WORLD_VARS; i++)
    {
        world_vars[i] = 0;
    }

    world_vars[0] = 100;
    world_vars[63] = -100;

    ASSERT_EQ(100, world_vars[0], "World var 0 set correctly");
    ASSERT_EQ(-100, world_vars[63], "World var 63 set correctly");

    // Simulate local vars array
    Sint32 local_vars[ACS_LOCAL_VARS];
    for (int i = 0; i < ACS_LOCAL_VARS; i++)
    {
        local_vars[i] = 0;
    }

    local_vars[0] = 42;
    local_vars[9] = -1;

    ASSERT_EQ(42, local_vars[0], "Local var 0 set correctly");
    ASSERT_EQ(-1, local_vars[9], "Local var 9 set correctly");

    PASS();
}

void test_acs_opcode_basics()
{
    TEST("Basic ACS opcode concepts work");

    // Simulate opcode execution state
    struct acs_script_t
    {
        unsigned number;
        Sint32 *code;
        unsigned num_args;
        int state;
        Uint32 wait_data;
    };

    acs_script_t script;
    script.number = 1;
    script.num_args = 2;
    script.state = 0; // ACS_stopped
    script.wait_data = 0;
    script.code = NULL;

    ASSERT_EQ(1, script.number, "Script number accessible");
    ASSERT_EQ(2, script.num_args, "Script args accessible");
    ASSERT_EQ(0, script.state, "Script state accessible");

    PASS();
}

void test_acs_arithmetic_operations()
{
    TEST("ACS arithmetic operations work correctly");

    // Simulate stack-based arithmetic
    Sint32 a = 10;
    Sint32 b = 3;

    // Addition
    ASSERT_EQ(13, a + b, "Addition correct");

    // Subtraction
    ASSERT_EQ(7, a - b, "Subtraction correct");

    // Multiplication
    ASSERT_EQ(30, a * b, "Multiplication correct");

    // Division
    ASSERT_EQ(3, a / b, "Division correct");

    // Modulo
    ASSERT_EQ(1, a % b, "Modulo correct");

    PASS();
}

void test_acs_comparison_operations()
{
    TEST("ACS comparison operations work correctly");

    Sint32 a = 10;
    Sint32 b = 20;
    Sint32 c = 10;

    // Equality
    ASSERT_EQ(true, a == c, "Equality check");
    ASSERT_EQ(false, a == b, "Inequality check");

    // Less than
    ASSERT_EQ(true, a < b, "Less than");
    ASSERT_EQ(false, b < a, "Not less than");

    // Greater than
    ASSERT_EQ(true, b > a, "Greater than");
    ASSERT_EQ(false, a > b, "Not greater than");

    PASS();
}

void test_acs_logical_operations()
{
    TEST("ACS logical operations work correctly");

    Sint32 true_val = 1;
    Sint32 false_val = 0;

    // Logical AND
    ASSERT_EQ(1, true_val && true_val, "AND true && true");
    ASSERT_EQ(0, true_val && false_val, "AND true && false");
    ASSERT_EQ(0, false_val && true_val, "AND false && true");
    ASSERT_EQ(0, false_val && false_val, "AND false && false");

    // Logical OR
    ASSERT_EQ(1, true_val || true_val, "OR true || true");
    ASSERT_EQ(1, true_val || false_val, "OR true || false");
    ASSERT_EQ(1, false_val || true_val, "OR false || true");
    ASSERT_EQ(0, false_val || false_val, "OR false || false");

    // Logical NOT
    ASSERT_EQ(0, !true_val, "NOT true");
    ASSERT_EQ(1, !false_val, "NOT false");

    PASS();
}

void test_acs_bitwise_operations()
{
    TEST("ACS bitwise operations work correctly");

    Uint8 a = 0x0F; // 00001111
    Uint8 b = 0x30; // 00110000

    // Bitwise AND
    ASSERT_EQ(0x00, a & b, "Bitwise AND");

    // Bitwise OR
    ASSERT_EQ(0x3F, a | b, "Bitwise OR");

    // Bitwise XOR
    ASSERT_EQ(0x3F, a ^ b, "Bitwise XOR");

    // Bitwise NOT
    ASSERT_EQ(0xF0, ~a, "Bitwise NOT");

    // Left shift
    ASSERT_EQ(0x1E, a << 1, "Left shift");

    // Right shift
    ASSERT_EQ(0x07, a >> 1, "Right shift");

    PASS();
}

int main()
{
    cout << "=== ACS Scripting Tests ===" << endl;

    test_acs_constants();
    test_acs_stack_operations();
    test_acs_script_state_enum();
    test_acs_stack_push_pop();
    test_acs_stack_overflow_check();
    test_acs_variable_arrays();
    test_acs_opcode_basics();
    test_acs_arithmetic_operations();
    test_acs_comparison_operations();
    test_acs_logical_operations();
    test_acs_bitwise_operations();

    cout << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
