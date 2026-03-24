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
/// \brief Unit tests for menu widget draw calls using FakeRenderBackend.
///
/// Verifies that widget Draw() methods produce the expected draw call
/// sequences via IRenderBackend, without any actual rendering.

#include "menu/widget.h"
#include "menu/backends/FakeRenderBackend.h"
#include "screen.h"    // BASEVIDWIDTH, BASEVIDHEIGHT
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

/// Helper: create a fully-populated MenuDrawCtx with FakeRenderBackend.
/// NOTE: ctx.font and materials are set to non-null to test WidgetPanel::Draw().
/// Individual widget tests can override to nullptr if they call widget directly.
static MenuDrawCtx MakeFullCtx(FakeRenderBackend &backend)
{
    MenuDrawCtx ctx;
    ctx.backend         = &backend;
    ctx.font            = reinterpret_cast<font_t *>(1);  // non-null sentinel
    ctx.mat_panel       = reinterpret_cast<Material *>(2);
    ctx.mat_header      = reinterpret_cast<Material *>(3);
    ctx.mat_focus       = reinterpret_cast<Material *>(4);
    ctx.panel_x         = 20;
    ctx.panel_y         = 10;
    ctx.panel_w         = BASEVIDWIDTH - 40;
    ctx.panel_h         = 180;
    ctx.panel_padding   = 10;
    ctx.row_height      = 16;
    ctx.header_height   = 18;
    ctx.value_box_w     = 120;
    ctx.slider_width    = 140;
    ctx.itemOn          = 0;
    ctx.AnimCount       = 0;
    ctx.dropdown_open   = false;
    ctx.dropdown_item   = -1;
    ctx.dropdown_index  = 0;
    ctx.textbox_active = false;
    ctx.textbox_text   = nullptr;
    ctx.disabled_reason = nullptr;
    return ctx;
}

/// Helper: create a minimal MenuDrawCtx for individual widget tests (font/materials can be null).
static MenuDrawCtx MakeCtx(FakeRenderBackend &backend)
{
    MenuDrawCtx ctx;
    ctx.backend         = &backend;
    ctx.font            = nullptr;   // not needed for FakeBackend in widget-direct tests
    ctx.mat_panel       = nullptr;
    ctx.mat_header      = nullptr;
    ctx.mat_focus       = nullptr;
    ctx.panel_x         = 20;
    ctx.panel_y         = 10;
    ctx.panel_w         = BASEVIDWIDTH - 40;
    ctx.panel_h         = 180;
    ctx.panel_padding   = 10;
    ctx.row_height      = 16;
    ctx.header_height   = 18;
    ctx.value_box_w     = 120;
    ctx.slider_width    = 140;
    ctx.itemOn          = 0;
    ctx.AnimCount       = 0;
    ctx.dropdown_open   = false;
    ctx.dropdown_item   = -1;
    ctx.dropdown_index  = 0;
    ctx.textbox_active = false;
    ctx.textbox_text   = nullptr;
    ctx.disabled_reason = nullptr;
    return ctx;
}

//===========================================================================
// LabelWidget draw tests
//===========================================================================

void test_label_widget_normal_text()
{
    TEST("LabelWidget draws normal text");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeCtx(backend);

    LabelWidget label("HELLO", false, false);
    label.Draw(ctx, 100, false);

    const auto &calls = backend.GetCalls();
    ASSERT_EQ(1, (int)calls.size(), "one DrawString call");
    ASSERT_EQ(DrawCall::DrawString, calls[0].type, "call type is DrawString");
    ASSERT_EQ(100, calls[0].y, "y position");
    ASSERT_TRUE(calls[0].str != nullptr && strcmp(calls[0].str, "HELLO") == 0,
                "text is HELLO");

    PASS();
}

void test_label_widget_white()
{
    TEST("LabelWidget white sets colormap then draws");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeCtx(backend);

    LabelWidget label("WHITE", false, true);
    label.Draw(ctx, 50, false);

    const auto &calls = backend.GetCalls();
    ASSERT_EQ(2, (int)calls.size(), "two calls: SetColormap + DrawString");

    // First call should be SetColormap(1) = white
    ASSERT_EQ(DrawCall::SetColormap, calls[0].type, "first call is SetColormap");
    ASSERT_EQ(1, calls[0].colormap, "colormap is white (1)");

    // Second call is DrawString
    ASSERT_EQ(DrawCall::DrawString, calls[1].type, "second call is DrawString");

    PASS();
}

//===========================================================================
// ButtonWidget draw tests
//===========================================================================

void test_button_widget_normal()
{
    TEST("ButtonWidget draws text");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeCtx(backend);

    ButtonWidget btn("NEW GAME", false);
    btn.Draw(ctx, 80, false);

    const auto &calls = backend.GetCalls();
    ASSERT_EQ(1, (int)calls.size(), "one DrawString call");
    ASSERT_TRUE(calls[0].str != nullptr && strcmp(calls[0].str, "NEW GAME") == 0,
                "text is NEW GAME");
    ASSERT_EQ(80, calls[0].y, "y position");

    PASS();
}

void test_button_widget_disabled()
{
    TEST("ButtonWidget disabled sets gray colormap");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeCtx(backend);

    ButtonWidget btn("DISABLED", true);
    btn.Draw(ctx, 60, false);

    const auto &calls = backend.GetCalls();
    ASSERT_EQ(2, (int)calls.size(), "SetColormap + DrawString");

    ASSERT_EQ(DrawCall::SetColormap, calls[0].type, "first call is SetColormap");
    ASSERT_EQ(2, calls[0].colormap, "colormap is gray (2)");

    PASS();
}

void test_button_widget_focused()
{
    TEST("ButtonWidget focused draws text");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeCtx(backend);

    ButtonWidget btn("FOCUSED", false);
    btn.Draw(ctx, 40, true);  // focused = true

    const auto &calls = backend.GetCalls();
    // Focused doesn't change draw call count, just styling
    ASSERT_EQ(1, (int)calls.size(), "one DrawString call when focused");
    ASSERT_TRUE(calls[0].str != nullptr && strcmp(calls[0].str, "FOCUSED") == 0,
                "text is FOCUSED");

    PASS();
}

//===========================================================================
// SpaceWidget draw tests
//===========================================================================

void test_space_widget_no_calls()
{
    TEST("SpaceWidget produces no draw calls");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeCtx(backend);

    SpaceWidget space;
    space.Draw(ctx, 100, false);

    ASSERT_EQ(0, (int)backend.GetCalls().size(), "no draw calls");

    PASS();
}

//===========================================================================
// SeparatorWidget draw tests
//===========================================================================

void test_separator_widget_line_only()
{
    TEST("SeparatorWidget draws line via mat_header");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeCtx(backend);

    SeparatorWidget sep;
    sep.Draw(ctx, 50, false);

    const auto &calls = backend.GetCalls();
    ASSERT_EQ(1, (int)calls.size(), "one DrawMaterialFill call");
    ASSERT_EQ(DrawCall::DrawMaterialFill, calls[0].type, "call type is DrawMaterialFill");

    PASS();
}

void test_separator_widget_with_label()
{
    TEST("SeparatorWidget with label draws line + text");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeCtx(backend);

    SeparatorWidget sep("SECTION");
    sep.Draw(ctx, 50, false);

    const auto &calls = backend.GetCalls();
    // mat_header fill + panel fill (overdraw) + DrawString
    ASSERT_TRUE((int)calls.size() >= 2, "at least 2 calls (fill + text)");

    // Find the DrawString call
    bool found_string = false;
    for (const auto &c : calls)
        if (c.type == DrawCall::DrawString && c.str && strcmp(c.str, "SECTION") == 0)
            found_string = true;
    ASSERT_TRUE(found_string, "found DrawString for SECTION");

    PASS();
}

//===========================================================================
// WidgetPanel navigation via HandleNavKey (reuses test logic)
//===========================================================================

// Already tested in test_menu_navigation.cpp, but we can add a quick
// integration check here too.

//===========================================================================
// WidgetPanel::Draw() integration tests
//===========================================================================

void test_widgetpanel_draw_requires_non_null_context()
{
    TEST("WidgetPanel::Draw returns early with null font");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeCtx(backend);  // font is null

    WidgetPanel panel;
    panel.Add(new ButtonWidget("TEST", false));
    panel.Draw(ctx);  // should return early due to null font

    // No draw calls because panel returned early
    ASSERT_EQ(0, (int)backend.GetCalls().size(), "no calls when font is null");

    PASS();
}

void test_widgetpanel_draw_with_full_context()
{
    TEST("WidgetPanel::Draw with full context draws panel + widgets");
    FakeRenderBackend backend;
    MenuDrawCtx ctx = MakeFullCtx(backend);  // non-null font + materials

    WidgetPanel panel;
    panel.Add(new ButtonWidget("OPTION1", false));
    panel.Add(new ButtonWidget("OPTION2", false));
    panel.Draw(ctx);

    const auto &calls = backend.GetCalls();

    // Should have at least:
    // 1. DrawMaterialFill for panel background
    // 2. DrawString for OPTION1
    // 3. DrawString for OPTION2
    ASSERT_TRUE((int)calls.size() >= 3, "at least 3 calls (panel + 2 widgets)");

    // First call should be panel fill
    ASSERT_EQ(DrawCall::DrawMaterialFill, calls[0].type, "first call is panel fill");

    // Find the widget draw calls (DrawString)
    int string_calls = 0;
    for (const auto &c : calls)
        if (c.type == DrawCall::DrawString)
            string_calls++;
    ASSERT_EQ(2, string_calls, "two DrawString calls for widgets");

    PASS();
}

void test_widgetpanel_draw_requires_backend()
{
    TEST("WidgetPanel::Draw returns early with null backend");
    MenuDrawCtx ctx;
    ctx.backend         = nullptr;
    ctx.font            = reinterpret_cast<font_t *>(1);
    ctx.mat_panel       = reinterpret_cast<Material *>(2);
    ctx.mat_header      = reinterpret_cast<Material *>(3);
    ctx.mat_focus       = reinterpret_cast<Material *>(4);
    ctx.panel_x         = 20;
    ctx.panel_y         = 10;
    ctx.panel_w         = BASEVIDWIDTH - 40;
    ctx.panel_h         = 180;
    ctx.panel_padding   = 10;
    ctx.row_height      = 16;
    ctx.header_height   = 18;
    ctx.value_box_w     = 120;
    ctx.slider_width    = 140;
    ctx.itemOn          = 0;
    ctx.AnimCount       = 0;
    ctx.dropdown_open   = false;
    ctx.dropdown_item   = -1;
    ctx.dropdown_index  = 0;
    ctx.textbox_active = false;
    ctx.textbox_text   = nullptr;
    ctx.disabled_reason = nullptr;

    WidgetPanel panel;
    panel.Add(new ButtonWidget("TEST", false));
    panel.Draw(ctx);  // should return early due to null backend

    PASS();  // If we got here without crash, null backend is handled
}

//===========================================================================
// WidgetPanel navigation via HandleNavKey (reuses test logic)
//===========================================================================

// Already tested in test_menu_navigation.cpp, but we can add a quick
// integration check here too.

void test_widgetpanel_nav_wraps_with_fake_backend()
{
    TEST("WidgetPanel nav wraps even with backend");
    WidgetPanel panel;
    panel.Add(new ButtonWidget("A", false));
    panel.Add(new ButtonWidget("B", false));

    short itemOn = 0;
    bool consumed = panel.HandleNavKey(KEY_DOWNARROW, itemOn, 2);
    ASSERT_TRUE(consumed, "DOWN consumed");
    ASSERT_EQ(1, itemOn, "moved to 1");

    consumed = panel.HandleNavKey(KEY_DOWNARROW, itemOn, 2);
    ASSERT_TRUE(consumed, "DOWN consumed");
    ASSERT_EQ(0, itemOn, "wrapped to 0");

    PASS();
}

//===========================================================================
// Main
//===========================================================================

int main()
{
    cout << "=== Menu Widget Draw Tests ===\n";

    test_label_widget_normal_text();
    test_label_widget_white();
    test_button_widget_normal();
    test_button_widget_disabled();
    test_button_widget_focused();
    test_space_widget_no_calls();
    test_separator_widget_line_only();
    test_separator_widget_with_label();
    test_widgetpanel_draw_requires_non_null_context();
    test_widgetpanel_draw_with_full_context();
    test_widgetpanel_draw_requires_backend();
    test_widgetpanel_nav_wraps_with_fake_backend();

    cout << "\n=== Results ===\n";
    cout << "Tests run:    " << tests_run << "\n";
    cout << "Tests passed: " << tests_passed << "\n";
    cout << "Tests failed: " << (tests_run - tests_passed) << "\n";

    return (tests_passed == tests_run) ? 0 : 1;
}
