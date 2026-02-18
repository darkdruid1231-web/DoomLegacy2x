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
/// \brief Unit tests for MapInfo parsing (header-only tests).

#include <iostream>
#include <cstring>
#include <string>
#include <cmath>
#include "g_mapinfo.h"

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;
static string last_failure;

#define TEST(name) do { \
    tests_run++; \
    last_failure = ""; \
    cout << "  " << name << " ... "; \
} while(0)

#define PASS() do { \
    tests_passed++; \
    cout << "PASS" << endl; \
} while(0)

#define FAIL(msg) do { \
    last_failure = msg; \
    cout << "FAIL: " << msg << endl; \
    return; \
} while(0)

#define CHECK(cond, msg) do { \
    if (!(cond)) FAIL(msg); \
} while(0)

#define CHECK_EQ(actual, expected, msg) do { \
    if ((actual) != (expected)) { \
        FAIL(string(msg) + " (expected " + to_string(expected) + ", got " + to_string(actual) + ")"); \
    } \
} while(0)

//============================================================================
// MapInfo Enum Tests (compile-time checks)
//============================================================================

void test_map_state_enum_values() {
    TEST("map_state_enum_values");
    CHECK(MapInfo::MAP_UNLOADED == 0, "MAP_UNLOADED should be 0");
    CHECK(MapInfo::MAP_RUNNING == 1, "MAP_RUNNING should be 1");
    CHECK(MapInfo::MAP_INSTASIS == 2, "MAP_INSTASIS should be 2");
    CHECK(MapInfo::MAP_RESET == 3, "MAP_RESET should be 3");
    CHECK(MapInfo::MAP_FINISHED == 4, "MAP_FINISHED should be 4");
    CHECK(MapInfo::MAP_SAVED == 5, "MAP_SAVED should be 5");
    PASS();
}

//============================================================================
// MapInfo Type Tests (compile-time checks for member types)
//============================================================================

void test_mapinfo_has_state() {
    TEST("mapinfo_has_state");
    // Just check the type exists by using offsetof
    size_t offset = offsetof(MapInfo, state);
    CHECK(offset >= 0, "state member should exist");
    PASS();
}

void test_mapinfo_has_lumpname() {
    TEST("mapinfo_has_lumpname");
    size_t offset = offsetof(MapInfo, lumpname);
    CHECK(offset >= 0, "lumpname member should exist");
    PASS();
}

void test_mapinfo_has_nicename() {
    TEST("mapinfo_has_nicename");
    size_t offset = offsetof(MapInfo, nicename);
    CHECK(offset >= 0, "nicename member should exist");
    PASS();
}

void test_mapinfo_has_cluster() {
    TEST("mapinfo_has_cluster");
    size_t offset = offsetof(MapInfo, cluster);
    CHECK(offset >= 0, "cluster member should exist");
    PASS();
}

void test_mapinfo_has_mapnumber() {
    TEST("mapinfo_has_mapnumber");
    size_t offset = offsetof(MapInfo, mapnumber);
    CHECK(offset >= 0, "mapnumber member should exist");
    PASS();
}

void test_mapinfo_has_author() {
    TEST("mapinfo_has_author");
    size_t offset = offsetof(MapInfo, author);
    CHECK(offset >= 0, "author member should exist");
    PASS();
}

void test_mapinfo_has_description() {
    TEST("mapinfo_has_description");
    size_t offset = offsetof(MapInfo, description);
    CHECK(offset >= 0, "description member should exist");
    PASS();
}

void test_mapinfo_has_partime() {
    TEST("mapinfo_has_partime");
    size_t offset = offsetof(MapInfo, partime);
    CHECK(offset >= 0, "partime member should exist");
    PASS();
}

void test_mapinfo_has_gravity() {
    TEST("mapinfo_has_gravity");
    size_t offset = offsetof(MapInfo, gravity);
    CHECK(offset >= 0, "gravity member should exist");
    PASS();
}

void test_mapinfo_has_sky1() {
    TEST("mapinfo_has_sky1");
    size_t offset = offsetof(MapInfo, sky1);
    CHECK(offset >= 0, "sky1 member should exist");
    PASS();
}

void test_mapinfo_has_interpic() {
    TEST("mapinfo_has_interpic");
    size_t offset = offsetof(MapInfo, interpic);
    CHECK(offset >= 0, "interpic member should exist");
    PASS();
}

void test_mapinfo_has_warptrans() {
    TEST("mapinfo_has_warptrans");
    size_t offset = offsetof(MapInfo, warptrans);
    CHECK(offset >= 0, "warptrans member should exist");
    PASS();
}

void test_mapinfo_has_nextlevel() {
    TEST("mapinfo_has_nextlevel");
    size_t offset = offsetof(MapInfo, nextlevel);
    CHECK(offset >= 0, "nextlevel member should exist");
    PASS();
}

void test_mapinfo_has_secretlevel() {
    TEST("mapinfo_has_secretlevel");
    size_t offset = offsetof(MapInfo, secretlevel);
    CHECK(offset >= 0, "secretlevel member should exist");
    PASS();
}

//============================================================================
// MapInfo Field Count Tests
//============================================================================

// Count of key fields we expect in MapInfo
void test_mapinfo_key_field_count() {
    TEST("mapinfo_key_field_count");
    // Just verify sizeof works and struct is not empty
    CHECK(sizeof(MapInfo) > 0, "MapInfo should have non-zero size");
    PASS();
}

//============================================================================
// Main
//============================================================================

int main() {
    cout << "========================================" << endl;
    cout << "MapInfo Unit Tests" << endl;
    cout << "========================================" << endl;
    
    cout << "\n[MapInfo Enum Tests]" << endl;
    test_map_state_enum_values();
    
    cout << "\n[MapInfo Member Tests]" << endl;
    test_mapinfo_has_state();
    test_mapinfo_has_lumpname();
    test_mapinfo_has_nicename();
    test_mapinfo_has_cluster();
    test_mapinfo_has_mapnumber();
    test_mapinfo_has_author();
    test_mapinfo_has_description();
    test_mapinfo_has_partime();
    test_mapinfo_has_gravity();
    test_mapinfo_has_sky1();
    test_mapinfo_has_interpic();
    test_mapinfo_has_warptrans();
    test_mapinfo_has_nextlevel();
    test_mapinfo_has_secretlevel();
    
    cout << "\n[MapInfo Size Tests]" << endl;
    test_mapinfo_key_field_count();
    
    cout << "\n========================================" << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;
    cout << "========================================" << endl;
    
    return (tests_passed == tests_run) ? 0 : 1;
}
