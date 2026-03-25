//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
//-----------------------------------------------------------------------------
///
/// \file
/// \brief Unit tests for WidgetPanel navigation logic.
///
/// Tests WidgetPanel::HandleNavKey() which handles UP/DOWN navigation,
/// wrapping at boundaries, and skipping non-focusable widgets.

#include "menu/widget.h"
#include "doomdef.h"    // BASEVIDWIDTH
#include "keys.h"       // KEY_DOWNARROW, KEY_UPARROW
#include <cstring>
#include <iostream>

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    cout << "  " << name << "... "; \
    tests_run++; \
} while(0)

#define PASS() do { \
    cout << "PASS\n"; \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    cout << "FAIL: " << msg << "\n"; \
} while(0)

#define ASSERT_EQ(expected, actual, msg) do { \
    if ((expected) != (actual)) { \
        FAIL(msg << " (expected " << (expected) << ", got " << (actual) << ")"); \
        return; \
    } \
} while(0)

#define ASSERT_TRUE(actual, msg) do { \
    if (!(actual)) { \
        FAIL(msg << " (expected true)"); \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(actual, msg) do { \
    if (actual) { \
        FAIL(msg << " (expected false)"); \
        return; \
    } \
} while(0)

//===========================================================================
// Navigation tests
//===========================================================================

/// Test: DOWN navigation wraps from last item to first.
void test_nav_wraps_down()
{
    TEST("nav wraps down");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("Item 0", false));
    panel.Add(new ButtonWidget("Item 1", false));
    panel.Add(new ButtonWidget("Item 2", false));

    short itemOn = 0;
    panel.HandleNavKey(KEY_DOWNARROW, itemOn, 3);
    ASSERT_EQ(1, itemOn, "after first DOWN");

    panel.HandleNavKey(KEY_DOWNARROW, itemOn, 3);
    ASSERT_EQ(2, itemOn, "after second DOWN");

    panel.HandleNavKey(KEY_DOWNARROW, itemOn, 3);
    ASSERT_EQ(0, itemOn, "after third DOWN (wraps)");

    PASS();
}

/// Test: UP navigation wraps from first item to last.
void test_nav_wraps_up()
{
    TEST("nav wraps up");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("Item 0", false));
    panel.Add(new ButtonWidget("Item 1", false));
    panel.Add(new ButtonWidget("Item 2", false));

    short itemOn = 0;
    panel.HandleNavKey(KEY_UPARROW, itemOn, 3);
    ASSERT_EQ(2, itemOn, "after first UP (wraps to last)");

    panel.HandleNavKey(KEY_UPARROW, itemOn, 3);
    ASSERT_EQ(1, itemOn, "after second UP");

    panel.HandleNavKey(KEY_UPARROW, itemOn, 3);
    ASSERT_EQ(0, itemOn, "after third UP");

    PASS();
}

/// Test: DOWN skips SpaceWidget (not focusable).
void test_nav_skips_space_widget()
{
    TEST("nav skips space widget");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("A", false));
    panel.Add(new SpaceWidget());   // not focusable
    panel.Add(new ButtonWidget("B", false));

    short itemOn = 0;
    panel.HandleNavKey(KEY_DOWNARROW, itemOn, 3);
    ASSERT_EQ(2, itemOn, "DOWN from 0 skips SpaceWidget, lands on 2");

    panel.HandleNavKey(KEY_DOWNARROW, itemOn, 3);
    ASSERT_EQ(0, itemOn, "DOWN from 2 wraps to 0");

    PASS();
}

/// Test: DOWN skips disabled widgets.
void test_nav_skips_disabled()
{
    TEST("nav skips disabled");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("Enabled", false));
    panel.Add(new ButtonWidget("Disabled A", true));  // disabled
    panel.Add(new ButtonWidget("Disabled B", true));  // disabled
    panel.Add(new ButtonWidget("Enabled 2", false));

    short itemOn = 0;
    panel.HandleNavKey(KEY_DOWNARROW, itemOn, 4);
    ASSERT_EQ(3, itemOn, "DOWN from 0 skips both disabled, lands on 3");

    panel.HandleNavKey(KEY_DOWNARROW, itemOn, 4);
    ASSERT_EQ(0, itemOn, "DOWN from 3 wraps to 0");

    PASS();
}

/// Test: UP skips non-focusable widgets.
void test_nav_up_skips_nonfocusable()
{
    TEST("nav up skips non-focusable");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("X", false));
    panel.Add(new LabelWidget("Header", true, false));  // not focusable
    panel.Add(new ButtonWidget("Y", false));

    short itemOn = 2;
    panel.HandleNavKey(KEY_UPARROW, itemOn, 3);
    ASSERT_EQ(0, itemOn, "UP from 2 skips LabelWidget, lands on 0");

    PASS();
}

/// Test: DOWN from last focusable item wraps to first.
void test_nav_down_stays_at_last_focusable()
{
    TEST("nav down stays at last focusable");
    WidgetPanel panel;
    panel.Add(new SpaceWidget());  // not focusable
    panel.Add(new ButtonWidget("Only One", false));

    short itemOn = 1;
    panel.HandleNavKey(KEY_DOWNARROW, itemOn, 2);
    ASSERT_EQ(1, itemOn, "DOWN from last focusable stays there (wraps but only one focusable)");

    PASS();
}

/// Test: non-NAV key returns false.
void test_nav_ignores_other_keys()
{
    TEST("nav ignores other keys");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("A", false));

    short itemOn = 0;
    bool consumed = panel.HandleNavKey('a', itemOn, 1);  // 'a' is not UP/DOWN
    ASSERT_FALSE(consumed, "non-NAV key returns false");
    ASSERT_EQ(0, itemOn, "itemOn unchanged");

    PASS();
}

/// Test: single focusable item — DOWN wraps back to it.
void test_nav_single_item()
{
    TEST("nav single item");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("Only", false));

    short itemOn = 0;
    bool consumed = panel.HandleNavKey(KEY_DOWNARROW, itemOn, 1);
    ASSERT_TRUE(consumed, "DOWN consumes key");
    ASSERT_EQ(0, itemOn, "wraps to same item");

    consumed = panel.HandleNavKey(KEY_UPARROW, itemOn, 1);
    ASSERT_TRUE(consumed, "UP consumes key");
    ASSERT_EQ(0, itemOn, "wraps to same item");

    PASS();
}

/// Test: all-widgets disabled — navigation finds no focusable.
void test_nav_all_disabled()
{
    TEST("nav all disabled");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("A", true));
    panel.Add(new ButtonWidget("B", true));

    short itemOn = 0;
    bool consumed = panel.HandleNavKey(KEY_DOWNARROW, itemOn, 2);
    ASSERT_FALSE(consumed, "DOWN returns false when no focusable");
    ASSERT_EQ(0, itemOn, "itemOn unchanged");

    PASS();
}

/// Test: ContentHeight sums widget heights correctly.
void test_content_height()
{
    TEST("content height");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("A", false));     // row_height (16)
    panel.Add(new SpaceWidget());                 // row_height (16)
    panel.Add(new ButtonWidget("B", false));     // row_height (16)
    // total = 16 + 16 + 16 = 48

    int h = panel.ContentHeight(16, 18);
    ASSERT_EQ(48, h, "ContentHeight sums correctly");

    PASS();
}

/// Test: ContentHeight handles null widgets.
void test_content_height_with_nulls()
{
    TEST("content height with nulls");
    WidgetPanel panel;
    panel.Add(nullptr);  // widget 0 is null
    panel.Add(nullptr);  // widget 1 is null
    int h = panel.ContentHeight(16, 18);
    ASSERT_EQ(32, h, "nulls contribute row_height each");

    PASS();
}

//===========================================================================
// Main
//===========================================================================

int main()
{
    cout << "=== Menu Navigation Tests ===\n";

    test_nav_wraps_down();
    test_nav_wraps_up();
    test_nav_skips_space_widget();
    test_nav_skips_disabled();
    test_nav_up_skips_nonfocusable();
    test_nav_down_stays_at_last_focusable();
    test_nav_ignores_other_keys();
    test_nav_single_item();
    test_nav_all_disabled();
    test_content_height();
    test_content_height_with_nulls();

    cout << "\n=== Results ===\n";
    cout << "Tests run:    " << tests_run << "\n";
    cout << "Tests passed: " << tests_passed << "\n";
    cout << "Tests failed: " << (tests_run - tests_passed) << "\n";

    return (tests_passed == tests_run) ? 0 : 1;
}
