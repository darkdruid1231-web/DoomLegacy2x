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
/// \brief Production implementation of IMenuBackend for the classic menu renderer.
///
/// Wraps the real global state: Menu::font, hud_font, materials, window_background,
/// window_border[], Menu::pointer[], and current_colormap.

#ifndef menu_productionmenubackend_h
#define menu_productionmenubackend_h 1

#include "IMenuBackend.h"
#include "r_data.h"     // materials, Material
#include "r_draw.h"     // window_background
#include "v_video.h"    // font_t, hud_font, Menu::font (accessed via extern)
#include "console.h"    // whitemap, graymap, current_colormap
#include "../menu_helpers.h"  // M_DrawSlider, M_DrawTextBox, M_DrawThermo

//===========================================================================
//  ProductionMenuBackend
//===========================================================================

/// Production implementation — delegates every call to the real globals.
///
/// This is a stateless adapter. It does not own or manage any resources;
/// all state lives in the globals it references.
class ProductionMenuBackend : public IMenuBackend
{
public:
    ProductionMenuBackend() = default;

    // --- Patch / material drawing ---

    void DrawPatch(const char *patchname, int x, int y, int flags) override
    {
        Material *m = materials.Get(patchname);
        if (m && !m->tex.empty() && m->tex[0].t)
            m->Draw(static_cast<float>(x), static_cast<float>(y), flags);
    }

    void DrawBackgroundFill(int x, int y, int w, int h) override
    {
        if (window_background)
            window_background->DrawFill(x, y, w, h);
    }

    // --- String / character drawing ---

    void DrawMenuFontString(int x, int y, const char *str, int flags) override
    {
        if (Menu::font)
            Menu::font->DrawString(static_cast<float>(x), static_cast<float>(y), str, flags);
    }

    void DrawHudFontString(int x, int y, const char *str, int flags) override
    {
        if (hud_font)
            hud_font->DrawString(static_cast<float>(x), static_cast<float>(y), str, flags);
    }

    float DrawHudFontCharacter(int x, int y, int c, int flags) override
    {
        if (hud_font)
            return hud_font->DrawCharacter(static_cast<float>(x), static_cast<float>(y), c, flags);
        return 0.f;
    }

    // --- Colormap state ---

    void SetColormap(int which) override
    {
        switch (which)
        {
            case 1: current_colormap = whitemap; break;
            case 2: current_colormap = graymap;  break;
            default: current_colormap = nullptr; break;
        }
    }

    // --- Helper drawing functions (call static helpers from menu.cpp) ---

    void DrawSlider(int x, int y, int range) override
    {
        M_DrawSlider(x, y, range);
    }

    void DrawTextBox(int x, int y, int width, int height) override
    {
        M_DrawTextBox(x, y, width, height);
    }

    void DrawThermo(int x, int y, const consvar_t *cv) override
    {
        M_DrawThermo(x, y, const_cast<consvar_t *>(cv));
    }

    // --- Menu cursor / pointer ---

    void DrawCursor(int x, int y, int index) override
    {
        if (index >= 0 && index < 2 && Menu::pointer[index])
            Menu::pointer[index]->Draw(static_cast<float>(x), static_cast<float>(y), V_SCALE);
    }

    // --- Font metrics ---

    float MenuFontStringWidth(const char *str) const override
    {
        return Menu::font ? Menu::font->StringWidth(str) : 0.f;
    }

    int MenuFontHeight() const override
    {
        return Menu::font ? Menu::font->Height() : 0;
    }

    float HudFontStringWidth(const char *str) const override
    {
        return hud_font ? hud_font->StringWidth(str) : 0.f;
    }
};

#endif // menu_productionmenubackend_h
