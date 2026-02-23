//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// Test program for C++ vs C parity verification
// Reference: legacy_one/trunk/src/
//
// This program verifies that the C++ implementation matches the C version.
// Tests FAIL when parity is not achieved - this serves as a roadmap
// for remaining implementation work.
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

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "d_main.h"
#include "d_ticcmd.h"
#include "doomtype.h"
#include "g_actor.h"
#include "g_mapinfo.h"
#include "info.h"
#include "m_fixed.h"
#include "savegame.h"

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
        return;                                                                                    \
    } while (0)

#define CHECK(cond, msg)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            FAIL(msg);                                                                             \
            return;                                                                                \
        }                                                                                          \
    } while (0)

#define CHECK_EQ(actual, expected, msg)                                                            \
    do                                                                                             \
    {                                                                                              \
        tests_run++;                                                                               \
        if ((actual) != (expected))                                                                \
        {                                                                                          \
            cout << "FAIL: " << msg << endl;                                                       \
            cout << "    Expected: " << (expected) << " (0x" << hex << (expected) << ")" << endl;  \
            cout << "    Got:      " << (actual) << " (0x" << (actual) << ")" << dec << endl;      \
            tests_failed++;                                                                        \
            return;                                                                                \
        }                                                                                          \
        cout << "PASS" << endl;                                                                    \
    } while (0)

//============================================================================
// EXPECTED VALUES FROM legacy_one/trunk/src/
// These should be defined in C++ headers - tests verify they match
//============================================================================

// From legacy_one/src/m_fixed.c
// FRACBITS = 16, FRACUNIT = 65536

// From legacy_one/src/d_ticcmd.h
// BT_ATTACK=1, BT_USE=2, BT_JUMP=4, BT_FLYDOWN=8

// From legacy_one/src/info.h (mobjtype_t)
// MT_PLAYER=0, MT_POSSESSED=1, MT_SHOTGUY=2, MT_CHAINGUY=3

// From legacy_one/src/info.h (mobjflag_t)
// MF_SOLID=0x0001, MF_SHOOTABLE=0x0002, MF_NOGRAVITY=0x0008

// From legacy_one/src/g_mapinfo.h
// MAP_UNLOADED=-1, MAP_RUNNING=0, CLUSTER_UNDEFINED=-1

// From legacy_one/src/console.h
// COLOR_BLACK=0, COLOR_WHITE=7, COLOR_RED=8, COLOR_GREEN=9, COLOR_BLUE=10
// Note: The legacy_one C version uses standard VGA palette indices (0-15)

#include "console.h"
// SAVE_VERSION=139

//============================================================================
// FIXED-POINT MATH TESTS (Reference: legacy_one/src/m_fixed.c)
//============================================================================

void test_fixed_add()
{
    TEST("fixed_add basic");
    fixed_t a(10), b(20);
    fixed_t c = a + b;
    CHECK(c.floor() == 30, "10 + 20 = 30");
    PASS();
}

void test_fixed_subtract()
{
    TEST("fixed_subtract basic");
    fixed_t a(30), b(10);
    fixed_t c = a - b;
    CHECK(c.floor() == 20, "30 - 10 = 20");
    PASS();
}

void test_fixed_multiply()
{
    TEST("fixed_multiply");
    fixed_t a(4), b(2);
    fixed_t c = a * b;
    CHECK(c.floor() == 8, "4 * 2 = 8");

    fixed_t d(100), e(50);
    fixed_t f = d * e;
    CHECK(f.floor() == 5000, "100 * 50 = 5000");
    PASS();
}

void test_fixed_divide()
{
    TEST("fixed_divide");
    fixed_t a(100), b(4);
    fixed_t c = a / b;
    CHECK(c.floor() == 25, "100 / 4 = 25");
    PASS();
}

void test_fixed_operators()
{
    TEST("fixed_operators");
    fixed_t a(10), b(20);

    CHECK((a + b).floor() == 30, "+ operator");
    CHECK((b - a).floor() == 10, "- operator");
    CHECK((a * b).floor() == 200, "* operator");
    CHECK((b / a).floor() == 2, "/ operator");

    CHECK(a < b, "< operator");
    CHECK(b > a, "> operator");
    CHECK(a == a, "== operator");
    PASS();
}

void test_fixed_fracbits()
{
    TEST("fixed_fracbits precision");
    fixed_t quarter(0.25f);
    CHECK(quarter.floor() == 0, "0.25 floor is 0");

    fixed_t half(0.5f);
    CHECK(half.floor() == 0, "0.5 floor is 0");

    fixed_t one(1.0f);
    CHECK(one.floor() == 1, "1.0 floor is 1");

    fixed_t two(2.0f);
    CHECK(two.floor() == 2, "2.0 floor is 2");
    PASS();
}

void test_fixed_min_max()
{
    TEST("fixed_min_max constants");
    CHECK(fixed_t::FMIN < 0, "FMIN is negative");
    CHECK(fixed_t::FMAX > 0, "FMAX is positive");
    CHECK(fixed_t::FMIN < fixed_t::FMAX, "FMIN < FMAX");
    CHECK(fixed_t::UNIT == (1 << 16), "UNIT = 65536");
    PASS();
}

//============================================================================
// TICCMD TESTS (Reference: legacy_one/src/d_ticcmd.h)
//============================================================================

void test_ticcmd_structure()
{
    TEST("ticcmd_structure");
    ticcmd_t cmd;
    memset(&cmd, 0, sizeof(cmd));

    cmd.forward = 100;
    cmd.side = -50;
    cmd.yaw = 18000;
    cmd.pitch = 0;
    cmd.buttons = ticcmd_t::BT_ATTACK;

    CHECK(cmd.forward == 100, "forward field exists");
    CHECK(cmd.side == -50, "side field exists");
    CHECK(cmd.yaw == 18000, "yaw field exists");
    CHECK(cmd.buttons & ticcmd_t::BT_ATTACK, "buttons field with BT_ATTACK");
    PASS();
}

void test_ticcmd_buttons_parity()
{
    // These MUST match legacy_one values exactly
    TEST("BT_ATTACK=1");
    CHECK_EQ(ticcmd_t::BT_ATTACK, 1, "BT_ATTACK must equal 1");
    TEST("BT_USE=2");
    CHECK_EQ(ticcmd_t::BT_USE, 2, "BT_USE must equal 2");
    TEST("BT_JUMP=4");
    CHECK_EQ(ticcmd_t::BT_JUMP, 4, "BT_JUMP must equal 4");
    TEST("BT_FLYDOWN=8");
    CHECK_EQ(ticcmd_t::BT_FLYDOWN, 8, "BT_FLYDOWN must equal 8");
}

//============================================================================
// ACTOR/MONSTER TESTS (Reference: legacy_one/src/info.c, p_enemy.c)
//============================================================================

void test_mobj_types_parity()
{
    // Expected values from legacy_one/src/info.h
    // If these fail, MT_* constants don't match C version

    cout << endl;
    cout << "  [MT_* mobjtype_t constants - expected values from legacy_one]" << endl;
    cout << "  MT_PLAYER should = 0" << endl;
    cout << "  MT_POSSESSED should = 1" << endl;
    cout << "  MT_SHOTGUY should = 2" << endl;
    cout << "  MT_CHAINGUY should = 3" << endl;
    cout << "  [Requires info.h with MT_* definitions]" << endl;

    CHECK_EQ(MT_PLAYER, 0, "MT_PLAYER must equal 0");
    CHECK_EQ(MT_POSSESSED, 1, "MT_POSSESSED must equal 1");
    CHECK_EQ(MT_SHOTGUY, 2, "MT_SHOTGUY must equal 2");
    CHECK_EQ(MT_CHAINGUY, 3, "MT_CHAINGUY must equal 3");

    TEST("MT_* parity check");
    PASS();
}

void test_mobj_flags_parity()
{
    cout << endl;
    cout << "  [MF_* mobjflag_t constants - expected values from legacy_one]" << endl;
    cout << "  MF_SOLID should = 0x0001" << endl;
    cout << "  MF_SHOOTABLE should = 0x0002" << endl;
    cout << "  MF_NOGRAVITY should = 0x0008" << endl;
    cout << "  [Requires g_actor.h with MF_* definitions]" << endl;

    CHECK_EQ(MF_SOLID, 0x0001, "MF_SOLID must equal 0x0001");
    CHECK_EQ(MF_SHOOTABLE, 0x0002, "MF_SHOOTABLE must equal 0x0002");
    CHECK_EQ(MF_NOGRAVITY, 0x0008, "MF_NOGRAVITY must equal 0x0008");

    TEST("MF_* parity check");
    PASS();
}

void test_mobj_flags2_parity()
{
    cout << endl;
    cout << "  [MF2_* mobjflag2_t constants - expected values from legacy_one]" << endl;
    cout << "  MF2_LOGRAV should = 0x0001" << endl;
    cout << "  MF2_WINDTHRUST should = 0x0002" << endl;
    cout << "  [Requires g_actor.h with MF2_* definitions]" << endl;

    CHECK_EQ(MF2_LOGRAV, 0x0001, "MF2_LOGRAV must equal 0x0001");
    CHECK_EQ(MF2_WINDTHRUST, 0x0002, "MF2_WINDTHRUST must equal 0x0002");

    TEST("MF2_* parity check");
    PASS();
}

//============================================================================
// MAP PARSING TESTS (Reference: legacy_one/src/g_mapinfo.c)
//============================================================================

void test_mapinfo_constants_parity()
{
    cout << endl;
    cout << "  [MAP_* constants - expected values from legacy_one]" << endl;
    cout << "  MAP_UNLOADED should = -1" << endl;
    cout << "  MAP_RUNNING should = 0" << endl;
    cout << "  [Requires g_mapinfo.h with MAP_* definitions]" << endl;

    // Test using the enum values from MapInfo::mapstate_e
    CHECK_EQ(MapInfo::MAP_UNLOADED, -1, "MAP_UNLOADED must equal -1");
    CHECK_EQ(MapInfo::MAP_RUNNING, 0, "MAP_RUNNING must equal 0");

    TEST("MAP_* parity check");
    PASS();
}

void test_mapcluster_constants_parity()
{
    cout << endl;
    cout << "  [CLUSTER_* constants - expected values from legacy_one]" << endl;
    cout << "  CLUSTER_UNDEFINED should = -1" << endl;
    cout << "  CLUSTER_EXTERNAL_TEXTURES should = 1" << endl;
    cout << "  CLUSTER_FOG should = 2" << endl;
    cout << "  [Requires g_mapinfo.h with CLUSTER_* definitions]" << endl;

    CHECK_EQ(CLUSTER_UNDEFINED, -1, "CLUSTER_UNDEFINED must equal -1");
    CHECK_EQ(CLUSTER_EXTERNAL_TEXTURES, 1, "CLUSTER_EXTERNAL_TEXTURES must equal 1");
    CHECK_EQ(CLUSTER_FOG, 2, "CLUSTER_FOG must equal 2");

    TEST("CLUSTER_* parity check");
    PASS();
}

//============================================================================
// CONSOLE/UI TESTS (Reference: legacy_one/src/console.c)
//============================================================================

void test_console_colors_parity()
{
    cout << endl;
    cout << "  [COLOR_* constants - expected values from legacy_one]" << endl;
    cout << "  COLOR_BLACK should = 0" << endl;
    cout << "  COLOR_WHITE should = 7" << endl;
    cout << "  COLOR_RED should = 4" << endl;
    cout << "  COLOR_GREEN should = 2" << endl;
    cout << "  COLOR_BLUE should = 1" << endl;

    CHECK_EQ(COLOR_BLACK, 0, "COLOR_BLACK must equal 0");
    CHECK_EQ(COLOR_WHITE, 7, "COLOR_WHITE must equal 7");
    CHECK_EQ(COLOR_RED, 4, "COLOR_RED must equal 4");
    CHECK_EQ(COLOR_GREEN, 2, "COLOR_GREEN must equal 2");
    CHECK_EQ(COLOR_BLUE, 1, "COLOR_BLUE must equal 1");

    TEST("COLOR_* parity check");
    PASS();
}

//============================================================================
// SAVE/LOAD TESTS (Reference: legacy_one/src/savegame.c)
//============================================================================

void test_savegame_constants_parity()
{
    cout << endl;
    cout << "  [SAVE_VERSION constant - expected value from legacy_one]" << endl;
    cout << "  SAVE_VERSION should = 139" << endl;
    cout << "  [Requires savegame.h with SAVE_VERSION definition]" << endl;

    CHECK_EQ(SAVE_VERSION, 139, "SAVE_VERSION must equal 139");

    TEST("SAVE_VERSION parity check");
    PASS();
}

//============================================================================
// DEMO RECORDING TESTS (Reference: legacy_one/src/d_main.c)
//============================================================================

void test_demo_constants_parity()
{
    cout << endl;
    cout << "  [DEMOMARKER_* constants - expected values from legacy_one]" << endl;
    cout << "  DEMOMARKER_PLAYER should = 0" << endl;
    cout << "  DEMOMARKER_TOTAL should = 1" << endl;
    cout << "  DEMOMARKER_END should = 255" << endl;
    cout << "  [Requires d_main.h with DEMOMARKER_* definitions]" << endl;

    CHECK_EQ(DEMOMARKER_PLAYER, 0, "DEMOMARKER_PLAYER must equal 0");
    CHECK_EQ(DEMOMARKER_TOTAL, 1, "DEMOMARKER_TOTAL must equal 1");
    CHECK_EQ(DEMOMARKER_END, 255, "DEMOMARKER_END must equal 255");

    TEST("DEMOMARKER_* parity check");
    PASS();
}

//============================================================================
// MAIN
//============================================================================

int main()
{
    cout << "========================================" << endl;
    cout << "C++ vs C Parity Tests" << endl;
    cout << "Reference: legacy_one/trunk/src/" << endl;
    cout << "========================================" << endl;
    cout << endl;
    cout << "These tests verify C++ implementation matches C version." << endl;
    cout << "FAILING tests indicate missing parity - implement to match C." << endl;
    cout << endl;

    cout << "[Fixed-Point Math Tests]" << endl;
    cout << "Reference: legacy_one/src/m_fixed.c" << endl;
    test_fixed_add();
    test_fixed_subtract();
    test_fixed_multiply();
    test_fixed_divide();
    test_fixed_operators();
    test_fixed_fracbits();
    test_fixed_min_max();

    cout << endl;
    cout << "[TICmd Tests]" << endl;
    cout << "Reference: legacy_one/src/d_ticcmd.h" << endl;
    test_ticcmd_structure();
    test_ticcmd_buttons_parity();

    cout << endl;
    cout << "[Actor/Monster Tests]" << endl;
    cout << "Reference: legacy_one/src/info.c, p_enemy.c" << endl;
    test_mobj_types_parity();
    test_mobj_flags_parity();
    test_mobj_flags2_parity();

    cout << endl;
    cout << "[Map Parsing Tests]" << endl;
    cout << "Reference: legacy_one/src/g_mapinfo.c" << endl;
    test_mapinfo_constants_parity();
    test_mapcluster_constants_parity();

    cout << endl;
    cout << "[Console/UI Tests]" << endl;
    cout << "Reference: legacy_one/src/console.c" << endl;
    test_console_colors_parity();

    cout << endl;
    cout << "[Save/Load Tests]" << endl;
    cout << "Reference: legacy_one/src/savegame.c" << endl;
    test_savegame_constants_parity();

    cout << endl;
    cout << "[Demo Recording Tests]" << endl;
    cout << "Reference: legacy_one/src/d_main.c" << endl;
    test_demo_constants_parity();

    cout << endl;
    cout << "========================================" << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " passed";
    if (tests_failed > 0)
    {
        cout << ", " << tests_failed << " FAILED";
    }
    cout << endl;
    cout << "========================================" << endl;

    if (tests_failed > 0)
    {
        cout << endl;
        cout << "IMPLEMENTATION ROADMAP:" << endl;
        cout << "----------------------" << endl;
        cout << "Failed tests indicate C++ constants missing or incorrect." << endl;
        cout << "Add/update headers to match legacy_one C values." << endl;
    }

    return (tests_failed > 0) ? 1 : 0;
}
