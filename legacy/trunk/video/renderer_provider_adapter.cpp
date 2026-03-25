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

// Forward declare R_SetViewport — not in r_render.h, defined in r_main.cpp
void R_SetViewport(int viewport);

class RendererProviderAdapter : public IRenderer {
public:
    void beginFrame() override
    {
        if (rendermode == render_opengl)
            oglrenderer->StartFrame();
    }

    void endFrame() override
    {
        if (rendermode == render_opengl)
            oglrenderer->FinishFrame();
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

    void setViewport(int viewport) override
    {
        if (rendermode == render_opengl)
            oglrenderer->SetViewport(viewport);
        else
            R_SetViewport(viewport);
    }

    void setPalette(uint8_t* palette) override
    {
        // Palette is managed through vid.SetPalette(num) at game level.
        // This method exists for interface completeness but the actual
        // palette activation is handled by Video::SetPalette() called elsewhere.
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
