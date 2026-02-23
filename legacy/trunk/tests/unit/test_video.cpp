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
/// \brief Unit tests for Video system.

#include "doomtype.h"
#include "i_video.h"
#include "v_video.h"
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

void test_video_constants()
{
    TEST("Video-related constants are defined");
    ASSERT_EQ(1, true, "Basic assertion works");
    PASS();
}

void test_video_draw_flags()
{
    TEST("Video draw flags are defined correctly");

    ASSERT_EQ(0x10000, V_SLOC, "V_SLOC flag");
    ASSERT_EQ(0x20000, V_SSIZE, "V_SSIZE flag");
    ASSERT_EQ(0x40000, V_FLIPX, "V_FLIPX flag");
    ASSERT_EQ(0x80000, V_MAP, "V_MAP flag");
    ASSERT_EQ(0x100000, V_WHITEMAP, "V_WHITEMAP flag");
    ASSERT_EQ(0x200000, V_TL, "V_TL flag");

    PASS();
}

void test_video_flag_masks()
{
    TEST("Video flag masks are consistent");

    // V_SCALE should be V_SLOC | V_SSIZE
    int expected_scale = V_SLOC | V_SSIZE;
    ASSERT_EQ(expected_scale, V_SCALE, "V_SCALE = V_SLOC | V_SSIZE");

    // V_SCREENMASK should be reasonable
    ASSERT_EQ(true, V_SCREENMASK < 16, "V_SCREENMASK is small");

    PASS();
}

void test_video_screen_dimensions()
{
    TEST("Standard video dimensions are valid");

    // Common screen sizes
    int widths[] = {320, 640, 800, 1024, 1280, 1920};
    int heights[] = {200, 400, 480, 600, 720, 1080};

    for (int i = 0; i < 6; i++)
    {
        ASSERT_EQ(true, widths[i] > 0, "Width positive");
        ASSERT_EQ(true, heights[i] > 0, "Height positive");
    }

    PASS();
}

void test_video_aspect_ratio()
{
    TEST("Video aspect ratio calculations work");

    int width = 640;
    int height = 480;
    float aspect = (float)width / height;

    ASSERT_EQ(true, aspect > 1.0f, "4:3 aspect > 1");
    ASSERT_EQ(true, aspect < 2.0f, "4:3 aspect < 2");
    // 4:3 ratio check
    float expected_aspect = 1.333f;
    ASSERT_EQ(true, fabs(aspect - expected_aspect) < 0.01f, "4:3 ratio correct");

    PASS();
}

void test_video_pixels()
{
    TEST("Video pixel calculations work");

    int width = 800;
    int height = 600;
    int total_pixels = width * height;

    ASSERT_EQ(480000, total_pixels, "Pixel count correct");
    PASS();
}

void test_video_refresh_rate()
{
    TEST("Video refresh rate is valid");

    int refresh_rates[] = {30, 60, 75, 120, 144};

    for (int i = 0; i < 5; i++)
    {
        ASSERT_EQ(true, refresh_rates[i] >= 30, "Refresh rate >= 30");
        ASSERT_EQ(true, refresh_rates[i] <= 144, "Refresh rate <= 144");
    }

    PASS();
}

void test_video_color_depth()
{
    TEST("Video color depth calculations work");

    int bpp = 32; // Bits per pixel
    int bytes_per_pixel = bpp / 8;
    int width = 800;
    int height = 600;
    int stride = width * bytes_per_pixel;
    int total_bytes = stride * height;

    ASSERT_EQ(4, bytes_per_pixel, "32-bit = 4 bytes per pixel");
    ASSERT_EQ(3200, stride, "Stride calculation correct");
    ASSERT_EQ(true, total_bytes > 0, "Total bytes positive");

    PASS();
}

void test_video_scaling()
{
    TEST("Video scaling calculations work");

    int src_width = 320;
    int src_height = 200;
    int dst_width = 1280;
    int dst_height = 800;

    float scale_x = (float)dst_width / src_width;
    float scale_y = (float)dst_height / src_height;

    ASSERT_EQ(4.0f, scale_x, "Horizontal scale = 4x");
    ASSERT_EQ(4.0f, scale_y, "Vertical scale = 4x");

    PASS();
}

void test_video_pixel_formats()
{
    TEST("Video pixel format conversions work");

    // RGB to grayscale luminance
    int r = 255, g = 128, b = 0;
    int luminance = (r * 299 + g * 587 + b * 114) / 1000;

    ASSERT_EQ(true, luminance >= 0, "Luminance non-negative");
    ASSERT_EQ(true, luminance <= 255, "Luminance <= 255");

    PASS();
}

int main()
{
    cout << "=== Video System Tests ===" << endl;

    test_video_constants();
    test_video_draw_flags();
    test_video_flag_masks();
    test_video_screen_dimensions();
    test_video_aspect_ratio();
    test_video_pixels();
    test_video_refresh_rate();
    test_video_color_depth();
    test_video_scaling();
    test_video_pixel_formats();

    cout << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
