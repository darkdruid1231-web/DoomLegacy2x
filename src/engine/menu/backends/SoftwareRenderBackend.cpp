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

#include "SoftwareRenderBackend.h"

#include "console.h"    // whitemap, graymap, current_colormap
#include "r_data.h"     // Material
#include "v_video.h"    // font_t

void SoftwareRenderBackend::DrawString(int x, int y, const char *str, int flags)
{
    if (font_)
        font_->DrawString(static_cast<float>(x), static_cast<float>(y), str, flags);
}

float SoftwareRenderBackend::DrawCharacter(int x, int y, int c, int flags)
{
    if (font_)
        return font_->DrawCharacter(static_cast<float>(x), static_cast<float>(y), c, flags);
    return 0.f;
}

void SoftwareRenderBackend::DrawMaterial(int x, int y, Material *mat, int flags)
{
    if (mat)
        mat->Draw(static_cast<float>(x), static_cast<float>(y), flags);
}

void SoftwareRenderBackend::DrawMaterialFill(int x, int y, int w, int h, Material *mat)
{
    if (mat)
        mat->DrawFill(x, y, w, h);
}

void SoftwareRenderBackend::SetColormap(int which)
{
    if (which == 1)
        current_colormap = whitemap;
    else if (which == 2)
        current_colormap = graymap;
    else
        current_colormap = nullptr;
}

float SoftwareRenderBackend::StringWidth(const char *str)
{
    if (font_)
        return font_->StringWidth(str);
    return 0.f;
}

float SoftwareRenderBackend::StringWidth(const char *str, int n)
{
    if (font_)
        return font_->StringWidth(str, n);
    return 0.f;
}

int SoftwareRenderBackend::FontHeight() const
{
    if (font_)
        return static_cast<int>(font_->Height());
    return 0;
}
