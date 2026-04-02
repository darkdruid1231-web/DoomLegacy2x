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
/// \brief Unit tests for pure-math logic in the GL sprite rendering pipeline.
///
/// These tests replicate the arithmetic extracted from spritepres_t::Draw()
/// (video/r_sprite.cpp) and DrawSpriteItem() (video/hardware/oglrenderer.cpp)
/// so regressions in the rotation formula, floor-clamp fix, or frame-clamp fix
/// are caught without requiring an OpenGL context or game initialization.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

using namespace std;

// ---------------------------------------------------------------------------
// Inline replicas of the constants used in the production code.
// Keep these in sync with: include/core/tables.h and include/info.h
// ---------------------------------------------------------------------------

typedef uint32_t angle_t;
static const angle_t ANG45      = 0x20000000u;  ///< tables.h
static const int     TFF_FRAMEMASK = 0x7fff;     ///< info.h

// ---------------------------------------------------------------------------
// Test harness (matches the pattern in tests/unit/test_actor.cpp exactly)
// ---------------------------------------------------------------------------

static int   tests_run    = 0;
static int   tests_passed = 0;
static string last_failure;

#define TEST(name)                                                                                 \
    do                                                                                             \
    {                                                                                              \
        tests_run++;                                                                               \
        last_failure = "";                                                                         \
        std::cout << "  " << name << " ... ";                                                      \
    } while (0)

#define PASS()                                                                                     \
    do                                                                                             \
    {                                                                                              \
        tests_passed++;                                                                            \
        std::cout << "PASS" << std::endl;                                                          \
    } while (0)

#define FAIL(msg)                                                                                  \
    do                                                                                             \
    {                                                                                              \
        last_failure = msg;                                                                        \
        std::cout << "FAIL: " << msg << std::endl;                                                 \
    } while (0)

#define ASSERT_EQ(expected, actual, msg)                                                           \
    do                                                                                             \
    {                                                                                              \
        if ((expected) != (actual))                                                                \
        {                                                                                          \
            FAIL(msg);                                                                             \
            std::cout << "    Expected: " << (expected) << ", Got: " << (actual) << std::endl;     \
            return;                                                                                \
        }                                                                                          \
    } while (0)

#define ASSERT_TRUE(cond, msg)                                                                     \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            FAIL(msg);                                                                             \
            return;                                                                                \
        }                                                                                          \
    } while (0)

// ---------------------------------------------------------------------------
// Replica of the rotation formula from spritepres_t::Draw()
// Source: video/r_sprite.cpp
//
//   float dx = p->pos.x.Float() - oglrenderer->x;
//   float dy = p->pos.y.Float() - oglrenderer->y;
//   angle_t ang = (angle_t)((double)atan2(dy, dx) * (0x80000000UL / M_PI));
//   unsigned int rot = (ang - p->yaw + (unsigned int)(ANG45/2)*9) >> 29;
//   mat = sprframe->tex[rot & 7];
// ---------------------------------------------------------------------------
static unsigned int compute_rot(float dx, float dy, angle_t actor_yaw)
{
    angle_t ang = (angle_t)((double)atan2(dy, dx) * (0x80000000UL / M_PI));
    unsigned int rot = (ang - actor_yaw + (unsigned int)(ANG45 / 2) * 9) >> 29;
    return rot & 7u;
}

// ---------------------------------------------------------------------------
// Tests — sprite rotation index
//
// Camera is at the origin.  Actor faces East (actor_yaw = 0).
// We verify all 8 cardinal/ordinal directions produce distinct rot values
// in [0,7], and that the complete set {0,1,2,3,4,5,6,7} is covered.
// ---------------------------------------------------------------------------

void test_sprite_rotation_index()
{
    // Expected rot values for each compass direction, actor_yaw = 0.
    // Computed analytically from the formula above; see comments in plan file.
    struct { const char *name; float dx; float dy; unsigned int expected_rot; } cases[] = {
        { "East  (1,0)",   1.0f,  0.0f, 4 },
        { "NE    (1,1)",   1.0f,  1.0f, 5 },
        { "North (0,1)",   0.0f,  1.0f, 6 },
        { "NW    (-1,1)", -1.0f,  1.0f, 7 },
        { "West  (-1,0)", -1.0f,  0.0f, 0 },
        { "SW    (-1,-1)",-1.0f, -1.0f, 1 },
        { "South (0,-1)",  0.0f, -1.0f, 2 },
        { "SE    (1,-1)",  1.0f, -1.0f, 3 },
    };

    bool seen[8] = {};

    for (int i = 0; i < 8; i++)
    {
        string test_name = string("rotation index: actor ") + cases[i].name;
        TEST(test_name.c_str());
        unsigned int rot = compute_rot(cases[i].dx, cases[i].dy, 0u);
        ASSERT_EQ(cases[i].expected_rot, rot, "rotation index mismatch");
        seen[rot] = true;
        PASS();
    }

    // All 8 rotation slots must be covered exactly once.
    TEST("rotation index: all 8 slots covered");
    for (int r = 0; r < 8; r++)
        ASSERT_TRUE(seen[r], "rotation slot not covered");
    PASS();
}

// ---------------------------------------------------------------------------
// Tests — sprite rotation invariants
//
// Rotating the actor-to-camera direction by ANG45 must shift rot by exactly 1.
// ---------------------------------------------------------------------------

void test_sprite_rotation_invariants()
{
    // Rotating dx/dy by 45° counterclockwise increments rot by 1 (mod 8).
    TEST("rotation invariant: each ANG45 step increments rot by 1");
    unsigned int prev_rot = compute_rot(1.0f, 0.0f, 0u);  // start East = rot 4
    // Verify for the next 7 steps (45° increments)
    struct { float dx; float dy; } dirs[7] = {
        {  1.0f,  1.0f }, // NE
        {  0.0f,  1.0f }, // N
        { -1.0f,  1.0f }, // NW
        { -1.0f,  0.0f }, // W
        { -1.0f, -1.0f }, // SW
        {  0.0f, -1.0f }, // S
        {  1.0f, -1.0f }, // SE
    };
    for (int i = 0; i < 7; i++)
    {
        unsigned int rot = compute_rot(dirs[i].dx, dirs[i].dy, 0u);
        unsigned int expected = (prev_rot + 1) & 7u;
        if (rot != expected)
        {
            FAIL("rot did not increment by 1 at 45-degree step");
            std::cout << "    Step " << i << ": expected " << expected
                      << ", got " << rot << std::endl;
            return;
        }
        prev_rot = rot;
    }
    PASS();

    // rot & 7 must always be in [0,7] — guaranteed by the mask, but verify.
    TEST("rotation invariant: result always in [0,7]");
    float angles[] = { 0.0f, 45.0f, 90.0f, 135.0f, 180.0f, 225.0f, 270.0f, 315.0f };
    for (int i = 0; i < 8; i++)
    {
        float rad = (float)(angles[i] * M_PI / 180.0);
        float dx  = cosf(rad);
        float dy  = sinf(rad);
        unsigned int rot = compute_rot(dx, dy, 0u);
        ASSERT_TRUE(rot <= 7u, "rot out of range [0,7]");
    }
    PASS();
}

// ---------------------------------------------------------------------------
// Replica of the Z-clamp formula from DrawSpriteItem()
// Source: video/hardware/oglrenderer.cpp
//
//   top    = mat->topoffs;
//   bottom = top - mat->worldheight;
//   float zoffset = (bottom < 0) ? -bottom : 0.0f;
//   top   += zoffset;
//   bottom = 0.0f;
// ---------------------------------------------------------------------------
struct ZClampResult { float top; float bottom; float zoffset; };

static ZClampResult compute_z_clamp(float topoffs, float worldheight)
{
    float top    = topoffs;
    float bottom = top - worldheight;
    float zoffset = (bottom < 0.0f) ? -bottom : 0.0f;
    top    += zoffset;
    bottom  = 0.0f;
    ZClampResult r = { top, bottom, zoffset };
    return r;
}

// ---------------------------------------------------------------------------
// Tests — sprite Z-clamp (floor penetration fix)
// ---------------------------------------------------------------------------

void test_sprite_z_clamp()
{
    // Case 1: topoffs == worldheight — no clipping needed.
    // Sprite sits exactly on floor with bottom=0.
    {
        TEST("z_clamp: topoffs == worldheight (no clipping)");
        ZClampResult r = compute_z_clamp(72.0f, 72.0f);
        ASSERT_EQ(0.0f, r.zoffset, "zoffset should be 0");
        ASSERT_EQ(72.0f, r.top, "top should be 72");
        ASSERT_EQ(0.0f, r.bottom, "bottom must always be 0");
        PASS();
    }

    // Case 2: topoffs < worldheight — sprite extends below floor, must be lifted.
    // Classic Doom monster (e.g. topoffs=68, worldheight=72 → 4px penetration).
    {
        TEST("z_clamp: topoffs < worldheight (4px floor penetration, lifted)");
        ZClampResult r = compute_z_clamp(68.0f, 72.0f);
        ASSERT_EQ(4.0f,  r.zoffset, "zoffset should be 4");
        ASSERT_EQ(72.0f, r.top,     "top should be worldheight after lift");
        ASSERT_EQ(0.0f,  r.bottom,  "bottom must always be 0");
        PASS();
    }

    // Case 3: topoffs > worldheight — sprite floats above floor, no change needed.
    {
        TEST("z_clamp: topoffs > worldheight (sprite floats, no clipping)");
        ZClampResult r = compute_z_clamp(80.0f, 72.0f);
        ASSERT_EQ(0.0f,  r.zoffset, "zoffset should be 0");
        ASSERT_EQ(80.0f, r.top,     "top should remain at topoffs");
        ASSERT_EQ(0.0f,  r.bottom,  "bottom must always be 0");
        PASS();
    }

    // Case 4: topoffs == 0 — extreme case (full sprite below floor, lifted by worldheight).
    {
        TEST("z_clamp: topoffs == 0 (all below floor, lifted by worldheight)");
        ZClampResult r = compute_z_clamp(0.0f, 72.0f);
        ASSERT_EQ(72.0f, r.zoffset, "zoffset should equal worldheight");
        ASSERT_EQ(72.0f, r.top,     "top should be worldheight");
        ASSERT_EQ(0.0f,  r.bottom,  "bottom must always be 0");
        PASS();
    }

    // Invariant: bottom is ALWAYS 0 after clamping (unconditional in the code).
    TEST("z_clamp invariant: bottom always equals 0.0f");
    float test_pairs[][2] = { {72,72}, {50,100}, {100,50}, {0,64}, {128,64} };
    for (int i = 0; i < 5; i++)
    {
        ZClampResult r = compute_z_clamp(test_pairs[i][0], test_pairs[i][1]);
        ASSERT_EQ(0.0f, r.bottom, "bottom not 0");
        ASSERT_TRUE(r.top > 0.0f, "top must be positive");
    }
    PASS();

    // Invariant: top_final == max(topoffs, worldheight).
    TEST("z_clamp invariant: top_final == max(topoffs, worldheight)");
    for (int i = 0; i < 5; i++)
    {
        float topoffs     = test_pairs[i][0];
        float worldheight = test_pairs[i][1];
        ZClampResult r    = compute_z_clamp(topoffs, worldheight);
        float expected_top = (topoffs > worldheight) ? topoffs : worldheight;
        ASSERT_EQ(expected_top, r.top, "top_final != max(topoffs, worldheight)");
    }
    PASS();
}

// ---------------------------------------------------------------------------
// Replica of frame-clamp logic from spritepres_t::Draw() and GetFrame()
// Source: video/r_sprite.cpp
//
//   Draw():     if (frame >= numframes) frame = numframes - 1;   // clamp
//   GetFrame(): if (frame >= numframes) return NULL;             // reject
// ---------------------------------------------------------------------------
static int draw_clamp_frame(int frame, int numframes)
{
    if (frame >= numframes)
        frame = numframes - 1;
    return frame;
}

static bool getframe_valid(int frame, int numframes)
{
    return (frame < numframes);  // returns non-NULL only when valid
}

// ---------------------------------------------------------------------------
// Tests — sprite frame bounds clamping
// ---------------------------------------------------------------------------

void test_sprite_frame_clamp()
{
    // Draw() behavior: clamp out-of-range frames to numframes-1.
    TEST("frame_clamp (Draw): valid frame unchanged");
    ASSERT_EQ(0, draw_clamp_frame(0, 8), "frame 0 of 8 should stay 0");
    PASS();

    TEST("frame_clamp (Draw): last valid frame unchanged");
    ASSERT_EQ(7, draw_clamp_frame(7, 8), "frame 7 of 8 should stay 7");
    PASS();

    TEST("frame_clamp (Draw): frame == numframes clamped to numframes-1");
    ASSERT_EQ(7, draw_clamp_frame(8, 8), "frame 8 of 8 should clamp to 7");
    PASS();

    TEST("frame_clamp (Draw): large out-of-range frame clamped");
    ASSERT_EQ(2, draw_clamp_frame(99, 3), "frame 99 of 3 should clamp to 2");
    PASS();

    // GetFrame() behavior: reject (return NULL) when frame >= numframes.
    TEST("frame_clamp (GetFrame): valid frame is accepted");
    ASSERT_TRUE(getframe_valid(0, 8),  "frame 0 of 8 should be accepted");
    ASSERT_TRUE(getframe_valid(7, 8),  "frame 7 of 8 should be accepted");
    PASS();

    TEST("frame_clamp (GetFrame): frame == numframes is rejected");
    ASSERT_TRUE(!getframe_valid(8, 8), "frame 8 of 8 should be rejected");
    PASS();

    TEST("frame_clamp (GetFrame): large out-of-range rejected");
    ASSERT_TRUE(!getframe_valid(99, 3), "frame 99 of 3 should be rejected");
    PASS();

    // TFF_FRAMEMASK extraction: lower 15 bits only.
    TEST("TFF_FRAMEMASK extracts frame number from flags word");
    ASSERT_EQ(0x0042, (0x8042 & TFF_FRAMEMASK), "mask of 0x8042 should give 0x0042");
    ASSERT_EQ(0x0005, (0x0005 & TFF_FRAMEMASK), "mask of 0x0005 should give 0x0005");
    ASSERT_EQ(0x0000, (0x8000 & TFF_FRAMEMASK), "mask of 0x8000 should give 0x0000");
    PASS();
}

// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Sprite Logic Tests ===" << std::endl;
    std::cout << "(pure-math replicas; no OpenGL or game init required)" << std::endl;
    std::cout << std::endl;

    std::cout << "[Rotation index formula — spritepres_t::Draw()]" << std::endl;
    test_sprite_rotation_index();
    test_sprite_rotation_invariants();
    std::cout << std::endl;

    std::cout << "[Z-clamp formula — DrawSpriteItem()]" << std::endl;
    test_sprite_z_clamp();
    std::cout << std::endl;

    std::cout << "[Frame bounds clamping — Draw() and GetFrame()]" << std::endl;
    test_sprite_frame_clamp();
    std::cout << std::endl;

    std::cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << std::endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
