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
/// \file
/// \brief Test double for menu rendering — records all draw calls for verification.
///
/// Used in unit tests to verify that widget draw call sequences match expected
/// values without requiring any actual rendering.

#ifndef menu_fakerenderbackend_h
#define menu_fakerenderbackend_h 1

#include "IRenderBackend.h"
#include <cstring>
#include <vector>
#include <string>

//===========================================================================
//  Recorded draw call types
//===========================================================================

struct DrawCall
{
    enum Type
    {
        DrawString,
        DrawMaterial,
        DrawMaterialFill,
        SetColormap
    };

    Type type;
    int x = 0, y = 0, w = 0, h = 0, flags = 0;
    const char *str = nullptr;
    void *mat = nullptr;   // Material* — stored as void* to avoid dragging in r_data.h
    int colormap = 0;      // 0=none, 1=white, 2=gray

    DrawCall() = default;
    explicit DrawCall(Type t) : type(t) {}
};

//===========================================================================
//  FakeRenderBackend
//===========================================================================

/// Test double that records every draw call issued by menu widgets.
///
/// Tests can inspect the recorded call list via GetCalls() and verify:
///   - Correct strings are drawn at correct positions
///   - Correct materials are used
///   - Colormap state transitions are correct
///
/// Usage:
///   FakeRenderBackend backend;
///   MenuDrawCtx ctx;
///   ctx.backend = &backend;
///   myWidget->Draw(ctx, 100, true);
///   auto& calls = backend.GetCalls();
///   // verify calls...
///   backend.Clear();
class FakeRenderBackend : public IRenderBackend
{
    std::vector<DrawCall> calls_;

public:
    FakeRenderBackend() = default;

    /// Returns all recorded draw calls.
    const std::vector<DrawCall> &GetCalls() const { return calls_; }

    /// Clears all recorded calls.
    void Clear() { calls_.clear(); }

    /// DrawString call recording.
    void DrawString(int x, int y, const char *str, int flags) override
    {
        DrawCall c(DrawCall::DrawString);
        c.x = x;
        c.y = y;
        c.str = str;
        c.flags = flags;
        calls_.push_back(c);
    }

    /// DrawCharacter call recording.
    float DrawCharacter(int x, int y, int /*c*/, int flags) override
    {
        DrawCall call(DrawCall::DrawString);
        call.x = x;
        call.y = y;
        call.str = nullptr;  // single char, not recorded as string
        call.flags = flags;
        calls_.push_back(call);
        return 8.f;  // typical char width in virtual units
    }

    /// DrawMaterial call recording.
    void DrawMaterial(int x, int y, Material *mat, int flags) override
    {
        DrawCall c(DrawCall::DrawMaterial);
        c.x = x;
        c.y = y;
        c.mat = static_cast<void *>(mat);
        c.flags = flags;
        calls_.push_back(c);
    }

    /// DrawMaterialFill call recording.
    void DrawMaterialFill(int x, int y, int w, int h, Material *mat) override
    {
        DrawCall c(DrawCall::DrawMaterialFill);
        c.x = x;
        c.y = y;
        c.w = w;
        c.h = h;
        c.mat = static_cast<void *>(mat);
        calls_.push_back(c);
    }

    /// SetColormap call recording.
    void SetColormap(int which) override
    {
        DrawCall c(DrawCall::SetColormap);
        c.colormap = which;
        calls_.push_back(c);
    }

    /// Stub — returns 0 for testing (real metric comes from font).
    float StringWidth(const char * /*str*/) override { return 0.f; }

    /// Stub — returns 0 for testing.
    float StringWidth(const char * /*str*/, int /*n*/) override { return 0.f; }

    /// Stub — returns 0 for testing.
    int FontHeight() const override { return 0; }

    // --- Test helper utilities ---

    /// Find the first DrawString call with matching text.
    const DrawCall *FindStringCall(const char *text) const
    {
        for (const auto &c : calls_)
            if (c.type == DrawCall::DrawString && c.str && strcmp(c.str, text) == 0)
                return &c;
        return nullptr;
    }

    /// Find all DrawMaterial calls for a given material name.
    /// Note: requires material name lookup which is implementation-specific.
    std::vector<const DrawCall *> FindMaterialCalls(const char * /*matname*/) const
    {
        // For tests that just verify material was drawn (not which one):
        std::vector<const DrawCall *> result;
        for (const auto &c : calls_)
            if (c.type == DrawCall::DrawMaterial)
                result.push_back(&c);
        return result;
    }

    /// Count calls of a specific type.
    int CountCalls(DrawCall::Type t) const
    {
        int n = 0;
        for (const auto &c : calls_)
            if (c.type == t)
                ++n;
        return n;
    }
};

#endif // menu_fakerenderbackend_h
