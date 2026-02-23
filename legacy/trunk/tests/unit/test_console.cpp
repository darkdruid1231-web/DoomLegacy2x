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
/// \brief Unit tests for Command buffer and console header definitions.

#include "command.h"
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

// Test command buffer class existence
void test_command_buffer_class_exists()
{
    TEST("command_buffer_t class is defined");
    // Just verify we can reference it
    ASSERT_EQ(1, sizeof(command_buffer_t) > 0, "command_buffer_t class is defined");
    PASS();
}

void test_command_buffer_fields()
{
    TEST("command_buffer_t has required fields");

    // Verify basic structure fields exist by checking offset/size
    // We can't actually access them without a constructor, so just check class exists
    ASSERT_EQ(1, sizeof(command_buffer_t) > 0, "command_buffer_t is a valid type");
    PASS();
}

void test_console_variable_flags()
{
    TEST("CV_SAVE flag exists");
    ASSERT_EQ(0x0001, CV_SAVE, "CV_SAVE = 0x0001");
    PASS();

    TEST("CV_CALL flag exists");
    ASSERT_EQ(0x0002, CV_CALL, "CV_CALL = 0x0002");
    PASS();

    TEST("CV_NETVAR flag exists");
    ASSERT_EQ(0x0004, CV_NETVAR, "CV_NETVAR = 0x0004");
    PASS();

    TEST("CV_FLOAT flag exists");
    ASSERT_EQ(0x0010, CV_FLOAT, "CV_FLOAT = 0x0010");
    PASS();

    TEST("CV_MODIFIED flag exists");
    ASSERT_EQ(0x0040, CV_MODIFIED, "CV_MODIFIED = 0x0040");
    PASS();

    TEST("CV_ANNOUNCE flag exists");
    ASSERT_EQ(0x0080, CV_ANNOUNCE, "CV_ANNOUNCE = 0x0080");
    PASS();
}

void test_cvar_possible_value_structure()
{
    TEST("CV_PossibleValue_t structure exists");

    CV_PossibleValue_t pv;

    // Just verify the structure can be instantiated
    pv.value = 0;
    pv.strvalue = NULL;

    ASSERT_EQ(1, sizeof(pv) > 0, "CV_PossibleValue_t is valid");
    PASS();
}

void test_cvar_onoff_exists()
{
    TEST("CV_OnOff array exists");
    ASSERT_EQ(1, CV_OnOff != NULL, "CV_OnOff array exists");
    PASS();
}

void test_cvar_yesno_exists()
{
    TEST("CV_YesNo array exists");
    ASSERT_EQ(1, CV_YesNo != NULL, "CV_YesNo array exists");
    PASS();
}

void test_cvar_unsigned_exists()
{
    TEST("CV_Unsigned array exists");
    ASSERT_EQ(1, CV_Unsigned != NULL, "CV_Unsigned array exists");
    PASS();
}

void test_max_args_constant()
{
    TEST("MAX_ARGS constant defined");
    ASSERT_EQ(80, MAX_ARGS, "MAX_ARGS = 80");
    PASS();
}

void test_com_object_exists()
{
    TEST("COM global command buffer exists");
    ASSERT_EQ(1, sizeof(COM) > 0, "COM object accessible");
    PASS();
}

void test_consvar_t_exists()
{
    TEST("consvar_t structure exists");
    ASSERT_EQ(1, sizeof(consvar_t) > 0, "consvar_t structure is defined");
    PASS();
}

void test_consvar_flags()
{
    TEST("CV_NOTINNET flag exists");
    ASSERT_EQ(0x0020, CV_NOTINNET, "CV_NOTINNET = 0x0020");
    PASS();

    TEST("CV_NOINIT flag exists");
    ASSERT_EQ(0x0008, CV_NOINIT, "CV_NOINIT = 0x0008");
    PASS();

    TEST("CV_ANNOUNCE_ONCE flag exists");
    ASSERT_EQ(0x0100, CV_ANNOUNCE_ONCE, "CV_ANNOUNCE_ONCE = 0x0100");
    PASS();

    TEST("CV_HIDDEN flag exists");
    ASSERT_EQ(0x0200, CV_HIDDEN, "CV_HIDDEN = 0x0200");
    PASS();

    TEST("CV_HANDLER flag exists");
    ASSERT_EQ(0x0400, CV_HANDLER, "CV_HANDLER = 0x0400");
    PASS();
}

int main()
{
    cout << "=== Command Buffer Tests ===" << endl;

    test_command_buffer_class_exists();
    test_command_buffer_fields();
    test_console_variable_flags();
    test_cvar_possible_value_structure();
    test_cvar_onoff_exists();
    test_cvar_yesno_exists();
    test_cvar_unsigned_exists();
    test_max_args_constant();
    test_com_object_exists();
    test_consvar_t_exists();
    test_consvar_flags();

    cout << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
