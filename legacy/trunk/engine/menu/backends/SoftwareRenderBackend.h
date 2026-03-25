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
/// \brief Production software rendering backend for menus.
///
/// Delegates all drawing to the existing font_t and Material APIs,
/// providing the same rendering that users see in software mode.

#ifndef menu_softwarerenderbackend_h
#define menu_softwarerenderbackend_h 1

#include "IRenderBackend.h"

class font_t;

//===========================================================================
//  SoftwareRenderBackend
//===========================================================================

/// Production IRenderBackend implementation for software rendering.
///
/// Wraps the existing font_t and Material APIs.  The draw call sequence
/// is identical to what widgets currently produce, so this is a drop-in
/// replacement that preserves existing behavior while enabling testability.
class SoftwareRenderBackend : public IRenderBackend
{
    font_t *font_ = nullptr;

public:
    SoftwareRenderBackend() = default;

    /// Set the active font (must be called before DrawString/DrawCharacter).
    void SetFont(font_t *f) { font_ = f; }

    /// Draw a null-terminated string via font_->DrawString().
    void DrawString(int x, int y, const char *str, int flags) override;

    /// Draw a single character via font_->DrawCharacter().
    float DrawCharacter(int x, int y, int c, int flags) override;

    /// Draw a material at position via mat->Draw().
    void DrawMaterial(int x, int y, Material *mat, int flags) override;

    /// Fill a rectangle with a material via mat->DrawFill().
    void DrawMaterialFill(int x, int y, int w, int h, Material *mat) override;

    /// Set active colormap (whitemap / graymap / nullptr).
    void SetColormap(int which) override;

    /// Forward to font_->StringWidth().
    float StringWidth(const char *str) override;

    /// Forward to font_->StringWidth(str, n).
    float StringWidth(const char *str, int n) override;

    /// Forward to font_->Height().
    int FontHeight() const override;
};

#endif // menu_softwarerenderbackend_h
