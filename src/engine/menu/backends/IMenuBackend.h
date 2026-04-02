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
/// \brief Abstract interface for the classic (non-widget) menu rendering.
///
/// Allows DrawMenu() to be tested in isolation without depending on actual
/// video output or global state (font, materials, current_colormap).
///
/// Two implementations are provided:
///   - ProductionMenuBackend  — delegates to globals (Menu::font, hud_font,
///                              materials, window_background, etc.)
///   - FakeMenuBackend        — test double, records draw calls for verification

#ifndef menu_imenubackend_h
#define menu_imenubackend_h 1

class Material;
struct menuitem_t;
struct consvar_t;

//===========================================================================
//  IMenuBackend interface
//===========================================================================

/// Abstract interface for the classic DrawMenu() renderer.
///
/// Each method corresponds to one rendering operation that DrawMenu() or
/// its helpers (M_DrawSlider, M_DrawTextBox, etc.) perform on globals.
/// By routing everything through this interface, DrawMenu() becomes stateless
/// and fully testable with a FakeMenuBackend.
class IMenuBackend
{
public:
    virtual ~IMenuBackend() = default;

    // -----------------------------------------------------------------------
    // Patch / material drawing
    // -----------------------------------------------------------------------

    /// Draw a named patch at (x, y) with V_* flags.
    /// The backend resolves \a patchname through materials.Get() internally.
    virtual void DrawPatch(const char *patchname, int x, int y, int flags) = 0;

    /// Fill a rectangle with the window_background material.
    virtual void DrawBackgroundFill(int x, int y, int w, int h) = 0;

    // -----------------------------------------------------------------------
    // String / character drawing
    // -----------------------------------------------------------------------

    /// Draw a string using Menu::font (the per-menu font set in Menu::Init).
    virtual void DrawMenuFontString(int x, int y, const char *str, int flags) = 0;

    /// Draw a string using hud_font (the HUD/console font).
    virtual void DrawHudFontString(int x, int y, const char *str, int flags) = 0;

    /// Draw a single character using hud_font, return width drawn.
    virtual float DrawHudFontCharacter(int x, int y, int c, int flags) = 0;

    // -----------------------------------------------------------------------
    // Colormap state
    // -----------------------------------------------------------------------

    /// Set the active colormap for subsequent text/material draws.
    /// 0 = normal, 1 = white (whitemap), 2 = gray (graymap).
    virtual void SetColormap(int which) = 0;

    // -----------------------------------------------------------------------
    // Helper drawing functions
    // -----------------------------------------------------------------------

    /// Draw a slider bar at (x, y) with given range [0..100].
    /// Internally calls materials.Get() for M_SLIDEL, M_SLIDEM, M_SLIDER, M_SLIDEC.
    virtual void DrawSlider(int x, int y, int range) = 0;

    /// Draw a text-box border+fill at (x, y) with given pixel dimensions.
    /// Internally uses window_border[] patches and window_background.
    virtual void DrawTextBox(int x, int y, int width, int height) = 0;

    /// Draw a "thermo" display for a cvar at (x, y).
    /// Internally calls materials.Get() and M_DrawThermo logic.
    virtual void DrawThermo(int x, int y, const consvar_t *cv) = 0;

    // -----------------------------------------------------------------------
    // Menu cursor / pointer
    // -----------------------------------------------------------------------

    /// Draw the menu cursor/skull at (x, y) using Menu::pointer[index].
    virtual void DrawCursor(int x, int y, int index) = 0;

    // -----------------------------------------------------------------------
    // Font metrics (for computing positions before drawing)
    // -----------------------------------------------------------------------

    /// Returns the width of a string in the Menu::font.
    virtual float MenuFontStringWidth(const char *str) const = 0;

    /// Returns the height of the Menu::font.
    virtual int MenuFontHeight() const = 0;

    /// Returns the width of a string in hud_font.
    virtual float HudFontStringWidth(const char *str) const = 0;
};

#endif // menu_imenubackend_h
