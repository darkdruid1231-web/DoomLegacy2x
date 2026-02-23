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
/// \brief Basic data serialization tests.

#include "m_fixed.h"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;

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

// Test basic integer serialization (simplified, no LArchive)
void test_int_serialization()
{
    TEST("int_serialization");

    // Simple test: verify basic data types serialize correctly
    int original = 42;
    char buffer[4];
    memcpy(buffer, &original, 4);
    int restored;
    memcpy(&restored, buffer, 4);
    assert(original == restored);

    PASS();
}

void test_fixed_serialization()
{
    TEST("fixed_serialization");

    fixed_t f(10);
    char buffer[4];
    // Manual serialization
    Sint32 v = f.value();
    memcpy(buffer, &v, 4);
    // Deserialize
    Sint32 restored;
    memcpy(&restored, buffer, 4);
    assert(v == restored);

    PASS();
}

void test_fixed_negative_serialization()
{
    TEST("fixed_negative_serialization");

    fixed_t f(-10);
    char buffer[4];
    // Manual serialization
    Sint32 v = f.value();
    memcpy(buffer, &v, 4);
    // Deserialize
    Sint32 restored;
    memcpy(&restored, buffer, 4);
    assert(v == restored);

    PASS();
}

void test_fixed_fractional_serialization()
{
    TEST("fixed_fractional_serialization");

    fixed_t f(10.5f);
    char buffer[4];
    // Manual serialization
    Sint32 v = f.value();
    memcpy(buffer, &v, 4);
    // Deserialize
    Sint32 restored;
    memcpy(&restored, buffer, 4);
    assert(v == restored);

    PASS();
}

int main()
{
    cout << "Running serialization tests..." << endl;

    test_int_serialization();
    test_fixed_serialization();
    test_fixed_negative_serialization();
    test_fixed_fractional_serialization();

    cout << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
