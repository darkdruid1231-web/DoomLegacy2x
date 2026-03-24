// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
//-----------------------------------------------------------------------------
//
// \file
/// \brief Test double for the classic DrawMenu() renderer.
///
/// Records all draw calls issued by DrawMenu() so tests can verify the
/// expected rendering sequence without any actual video output.
///
/// Usage:
///   FakeMenuBackend backend;
///   DrawMenu(&backend, menu, itemOn, which_pointer, AnimCount);
///   auto& calls = backend.GetCalls();
///   // verify calls...

#ifndef menu_fakemenubackend_h
#define menu_fakemenubackend_h 1

#include "IMenuBackend.h"
#include <cstring>
#include <vector>
#include <string>

//===========================================================================
//  Recorded draw call types for the classic menu renderer
//===========================================================================

struct OldMenuCall
{
    enum Type
    {
        Patch,
        BackgroundFill,
        MenuFontString,
        HudFontString,
        HudFontCharacter,
        SetColormap,
        Slider,
        TextBox,
        Thermo,
        Cursor
    };

    Type type;
    int x = 0, y = 0, w = 0, h = 0, flags = 0;
    int range = 0;           // Slider
    int c = 0;               // HudFontCharacter
    int index = 0;            // Cursor (which_pointer)
    const char *str = nullptr;  // string content
    const char *patchname = nullptr;  // for Patch
    const void *cv = nullptr; // for Thermo

    OldMenuCall() = default;
    explicit OldMenuCall(Type t) : type(t) {}
};

//===========================================================================
//  FakeMenuBackend
//===========================================================================

/// Test double that records every draw call from DrawMenu().
///
/// Tests can inspect the recorded call list via GetCalls() and verify:
///   - Correct patch names are drawn at correct positions
///   - Correct strings are drawn with the correct font
///   - Colormap state transitions are correct
///   - Slider/TextBox/Thermo/Cursor calls have correct parameters
class FakeMenuBackend : public IMenuBackend
{
    std::vector<OldMenuCall> calls_;

public:
    FakeMenuBackend() = default;

    /// Returns all recorded draw calls.
    const std::vector<OldMenuCall> &GetCalls() const { return calls_; }

    /// Clears all recorded calls.
    void Clear() { calls_.clear(); }

    // --- IMenuBackend implementation (record-only) ---

    void DrawPatch(const char *patchname, int x, int y, int flags) override
    {
        OldMenuCall c(OldMenuCall::Patch);
        c.patchname = patchname;
        c.x = x;
        c.y = y;
        c.flags = flags;
        calls_.push_back(c);
    }

    void DrawBackgroundFill(int x, int y, int w, int h) override
    {
        OldMenuCall c(OldMenuCall::BackgroundFill);
        c.x = x; c.y = y; c.w = w; c.h = h;
        calls_.push_back(c);
    }

    void DrawMenuFontString(int x, int y, const char *str, int flags) override
    {
        OldMenuCall c(OldMenuCall::MenuFontString);
        c.x = x; c.y = y; c.str = str; c.flags = flags;
        calls_.push_back(c);
    }

    void DrawHudFontString(int x, int y, const char *str, int flags) override
    {
        OldMenuCall c(OldMenuCall::HudFontString);
        c.x = x; c.y = y; c.str = str; c.flags = flags;
        calls_.push_back(c);
    }

    float DrawHudFontCharacter(int x, int y, int c, int flags) override
    {
        OldMenuCall call(OldMenuCall::HudFontCharacter);
        call.x = x; call.y = y; call.c = c; call.flags = flags;
        calls_.push_back(call);
        return 8.f;  // typical char width in virtual units
    }

    void SetColormap(int which) override
    {
        OldMenuCall c(OldMenuCall::SetColormap);
        c.flags = which;  // reuse flags field: 1=white, 2=gray
        calls_.push_back(c);
    }

    void DrawSlider(int x, int y, int range) override
    {
        OldMenuCall c(OldMenuCall::Slider);
        c.x = x; c.y = y; c.range = range;
        calls_.push_back(c);
    }

    void DrawTextBox(int x, int y, int w, int h) override
    {
        OldMenuCall c(OldMenuCall::TextBox);
        c.x = x; c.y = y; c.w = w; c.h = h;
        calls_.push_back(c);
    }

    void DrawThermo(int x, int y, const consvar_t *cv) override
    {
        OldMenuCall c(OldMenuCall::Thermo);
        c.x = x; c.y = y; c.cv = static_cast<const void *>(cv);
        calls_.push_back(c);
    }

    void DrawCursor(int x, int y, int index) override
    {
        OldMenuCall c(OldMenuCall::Cursor);
        c.x = x; c.y = y; c.index = index;
        calls_.push_back(c);
    }

    // --- Font metrics (stubs for testing) ---

    float MenuFontStringWidth(const char * /*str*/) const override { return 0.f; }
    int MenuFontHeight() const override { return 0; }
    float HudFontStringWidth(const char * /*str*/) const override { return 0.f; }

    // --- Test helper utilities ---

    /// Find first call of a given type.
    const OldMenuCall *FindCall(OldMenuCall::Type t) const
    {
        for (const auto &c : calls_)
            if (c.type == t) return &c;
        return nullptr;
    }

    /// Count calls of a specific type.
    int CountCalls(OldMenuCall::Type t) const
    {
        int n = 0;
        for (const auto &c : calls_)
            if (c.type == t) ++n;
        return n;
    }

    /// Find first string call with matching text (any font).
    const OldMenuCall *FindStringCall(const char *text) const
    {
        for (const auto &c : calls_)
            if ((c.type == OldMenuCall::MenuFontString || c.type == OldMenuCall::HudFontString)
                && c.str && strcmp(c.str, text) == 0)
                return &c;
        return nullptr;
    }

    /// Find first Patch call with matching patch name.
    const OldMenuCall *FindPatchCall(const char *name) const
    {
        for (const auto &c : calls_)
            if (c.type == OldMenuCall::Patch && c.patchname && strcmp(c.patchname, name) == 0)
                return &c;
        return nullptr;
    }

    /// Find all calls of a specific type.
    std::vector<const OldMenuCall *> FindAll(OldMenuCall::Type t) const
    {
        std::vector<const OldMenuCall *> result;
        for (const auto &c : calls_)
            if (c.type == t) result.push_back(&c);
        return result;
    }
};

#endif // menu_fakemenubackend_h
