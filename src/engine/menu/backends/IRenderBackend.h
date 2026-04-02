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
///file
/// brief Abstract interface for menu rendering backends.
///
/// Allows the menu system to render without depending on actual video output,
/// enabling test doubles that record and verify draw call sequences.

#ifndef menu_irenderbackend_h
#define menu_irenderbackend_h 1

class Material;

//===========================================================================
//  IRenderBackend interface
//===========================================================================

/// Abstract interface for menu rendering.
///
/// All menu widgets (LabelWidget, ButtonWidget, etc.) delegate their
/// drawing through this interface.  Two implementations are provided:
///   - SoftwareRenderBackend  — production path, delegates to font_t/Material
///   - FakeRenderBackend       — test double, records draw calls for verification
class IRenderBackend
{
public:
    virtual ~IRenderBackend() = default;

    /// Draw a null-terminated string.
    virtual void DrawString(int x, int y, const char *str, int flags) = 0;

    /// Draw a single character, return width drawn.
    virtual float DrawCharacter(int x, int y, int c, int flags) = 0;

    /// Draw a named material at the given position.
    virtual void DrawMaterial(int x, int y, Material *mat, int flags) = 0;

    /// Fill a rectangle with a material (tiled).
    virtual void DrawMaterialFill(int x, int y, int w, int h, Material *mat) = 0;

    /// Set the active colormap for subsequent text/material draws.
    /// 0 = none (normal), 1 = white, 2 = gray (disabled).
    virtual void SetColormap(int which) = 0;

    /// Returns the width of a string in virtual units.
    virtual float StringWidth(const char *str) = 0;

    /// Returns the width of the first n characters of a string in virtual units.
    virtual float StringWidth(const char *str, int n) = 0;

    /// Returns the font height in virtual units.
    virtual int FontHeight() const = 0;
};

#endif // menu_irenderbackend_h
