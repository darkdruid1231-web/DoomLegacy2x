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
/// \brief IRenderer interface — abstracts OpenGL and software rendering.

#ifndef i_renderer_h
#define i_renderer_h

#include <cstdint>

class PlayerInfo;  // forward declaration

/// \brief Abstracts the rendering subsystem (OpenGL and software).
/// \details Follows the INetworkProvider pattern: abstract interface + static adapter
/// accessed via GetGlobalRenderer().
class IRenderer {
public:
    virtual ~IRenderer() = default;

    /// Called at the start of each frame, before any world/hud rendering.
    virtual void beginFrame() = 0;

    /// Called at the end of each frame, after all world/hud rendering.
    virtual void endFrame() = 0;

    /// Render the world from the given player's point of view.
    virtual void renderPlayerView(PlayerInfo* player) = 0;

    /// Switch to 2D coordinate mode for HUD/console drawing.
    virtual void setup2DMode() = 0;

    /// Switch to full-screen 2D mode (no pillarboxing).
    virtual void setupFullScreen2D() = 0;

    /// Set the palette (256-entry RGB array).
    virtual void setPalette(uint8_t* palette) = 0;

    /// Change resolution and recreate resources.
    virtual void changeResolution(int width, int height) = 0;

    /// Returns true if the renderer is initialized and ready to draw.
    virtual bool isReady() const = 0;

    /// Returns current render mode (render_soft=1 or render_opengl=2).
    virtual int getRenderMode() const = 0;
};

/// Get the global IRenderer instance.
IRenderer* GetGlobalRenderer();

#endif  // i_renderer_h
