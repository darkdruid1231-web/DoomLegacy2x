// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief IRenderer adapter — dispatches between OpenGL and software rendering.

#include "interfaces/i_renderer.h"

#include "i_video.h"
#include "i_system.h"
#include "screen.h"
#include "r_render.h"

#include "hardware/oglrenderer.hpp"

// Forward declare PlayerInfo — included via g_player.h in adapter
class PlayerInfo;

class RendererProviderAdapter : public IRenderer {
public:
    void beginFrame() override
    {
        I_StartFrame();
    }

    void endFrame() override
    {
        I_FinishUpdate();
    }

    void renderPlayerView(PlayerInfo* player) override
    {
        if (rendermode == render_opengl)
            oglrenderer->RenderPlayerView(player);
        else
            R.R_RenderPlayerView(player);
    }

    void setup2DMode() override
    {
        if (rendermode == render_opengl)
            oglrenderer->Setup2DMode();
    }

    void setupFullScreen2D() override
    {
        if (rendermode == render_opengl)
            oglrenderer->Setup2DMode_Full();
    }

    void setPalette(uint8_t* palette) override
    {
        // palette is 256 RGB_t entries; vid.SetPalette takes palettenum index
        // The caller should have loaded the palette already; we just activate it
        if (rendermode == render_opengl && oglrenderer)
        {
            oglrenderer->palette = reinterpret_cast<RGB_t*>(palette);
        }
        // For software mode, palette is managed through vid.SetPalette(num)
        // which is handled at game level via D_PaletteChanged event
        (void)palette;
    }

    void changeResolution(int width, int height) override
    {
        vid.width = width;
        vid.height = height;
        vid.setmodeneeded = 1;  // Signal mode change for next frame
    }

    bool isReady() const override
    {
        if (rendermode == render_opengl)
            return oglrenderer && oglrenderer->ReadyToDraw();
        // Software renderer is always "ready" once I_StartupGraphics succeeded
        return true;
    }

    int getRenderMode() const override
    {
        return rendermode;
    }
};

static RendererProviderAdapter s_rendererProviderAdapter;

IRenderer* GetGlobalRenderer()
{
    return &s_rendererProviderAdapter;
}
