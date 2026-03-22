// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: v_video.cpp 520 2008-01-07 01:37:12Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2007 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Texture blitting, blitting rectangles between buffers.

#include <ctype.h>

#include "command.h"
#include "console.h"
#include "doomdef.h"

#include "r_data.h"
#include "r_draw.h"

#include "screen.h"
#include "v_video.h"

#include "i_video.h"
#include "z_zone.h"

#include "hardware/oglrenderer.hpp"

using namespace std;

byte *current_colormap; // for applying colormaps to Drawn Textures

//=================================================================
//                    2D Drawing of Textures
//=================================================================

void Material::Draw(float x, float y, int scrn)
{
    if (rendermode == render_opengl)
    {
        if (oglrenderer && oglrenderer->ReadyToDraw())
            // Console tries to use some patches before graphics are
            // initialized. If this is the case, then create the missing
            // texture.
            oglrenderer->Draw2DGraphic_Doom(x, y, this, scrn & V_FLAGMASK);
        return;
    }

    bool masked = tex[0].t->Masked(); // means column_t based, in which case the current algorithm
                                      // cannot handle y clipping

    int flags = scrn & V_FLAGMASK;
    scrn &= V_SCREENMASK;
    byte *dest_tl = vid.screens[scrn] + vid.scaledofs; // destination top left in LFB

    // location scaling
    if (flags & V_SLOC)
    {
        x *= vid.dupx;
        y *= vid.dupy;
    }

    // size scaling
    fixed_t colfrac = tex[0].xscale;
    fixed_t rowfrac = tex[0].yscale;

    // clipping to LFB
    int x2, y2; // clipped past-the-end coords of the lower right corner in LFB

    if (flags & V_SSIZE)
    {
        x -= leftoffs * vid.dupx;
        y -= topoffs * vid.dupy;
        x2 = min(int(x + worldwidth * vid.dupx), vid.width);
        y2 = masked ? int(y + worldheight * vid.dupy)
                    : min(int(y + worldheight * vid.dupy), vid.height);
        colfrac /= vid.dupx;
        rowfrac /= vid.dupy;
    }
    else
    {
        x -= leftoffs;
        y -= topoffs;
        x2 = min(int(x + worldwidth), vid.width);
        y2 = masked ? int(y + worldheight) : min(int(y + worldheight), vid.height);
    }

    // clipped upper left corner
    int x1 = max(int(x), 0);
    int y1 = masked ? int(y) : max(int(y), 0);

#ifdef RANGECHECK
    if (y < 0 || y2 > vid.height)
    {
        fprintf(stderr, "Patch at (%f,%f) exceeds LFB.\n", x, y);
        // No I_Error abort - what is up with TNT.WAD?
        return;
    }
#endif

    // limits in LFB
    dest_tl += y1 * vid.width + x1;                  // top left
    byte *dest_tr = dest_tl + x2 - x1;               // top right, past-the-end
    byte *dest_bl = dest_tl + (y2 - y1) * vid.width; // bottom left, past-the-end

    // starting location in texture space
    fixed_t col;
    if (flags & V_FLIPX)
    {
        col = (x2 - x - 1) * colfrac; // a little back from right edge
        colfrac = -colfrac;           // reverse stride
    }
    else
    {
        col = (x1 - x) * colfrac;
    }

    fixed_t row = (y1 - y) * rowfrac;

    tex[0].t->Draw(dest_tl, dest_tr, dest_bl, col, row, colfrac, rowfrac, flags);
}

void PatchTexture::Draw(byte *dest_tl,
                        byte *dest_tr,
                        byte *dest_bl,
                        fixed_t col,
                        fixed_t row,
                        fixed_t colfrac,
                        fixed_t rowfrac,
                        int flags)
{
    // Defensive: check for null patch data
    if (!patch_data)
        return;

    patch_t *p = GeneratePatch();
    if (!p)
        return;

    for (; dest_tl < dest_tr; col += colfrac, dest_tl++)
    {
#warning FIXME LFB top/bottom limits
        post_t *post = reinterpret_cast<post_t *>(patch_data + p->columnofs[col.floor()]);

        // step through the posts in a column
        while (post->topdelta != 0xff)
        {
            // step through the posts in a column
            byte *dest = dest_tl + (post->topdelta / rowfrac).floor() * vid.width;
            byte *source = post->data;
            int count = (post->length / rowfrac).floor();

            fixed_t row = 0;
            while (count--)
            {
                byte pixel = source[row.floor()];

                // the compiler is supposed to optimize the ifs out of the loop
                if (flags & V_MAP)
                    pixel = current_colormap[pixel];

                if (flags & V_TL)
                    pixel = transtables[0][(pixel << 8) + *dest];

                *dest = pixel;
                dest += vid.width;
                row += rowfrac;
            }
            post = reinterpret_cast<post_t *>(&post->data[post->length + 1]); // next post
        }
    }
}

void LumpTexture::Draw(byte *dest_tl,
                       byte *dest_tr,
                       byte *dest_bl,
                       fixed_t col,
                       fixed_t startrow,
                       fixed_t colfrac,
                       fixed_t rowfrac,
                       int flags)
{
    byte *base = GetData(); // in col-major order!

    for (; dest_tl < dest_tr; col += colfrac, dest_tl++, dest_bl++)
    {
        byte *source = base + height * col.floor();

        fixed_t row = startrow;
        for (byte *dest = dest_tl; dest < dest_bl; row += rowfrac, dest += vid.width)
        {
            byte pixel = source[row.floor()];

            // the compiler is supposed to optimize the ifs out of the loop
            if (flags & V_MAP)
                pixel = current_colormap[pixel];

            if (flags & V_TL)
                pixel = transtables[0][(pixel << 8) + *dest];

            *dest = pixel;
        }
    }
}

// Draw the material stretched to fill exactly the given screen pixel rectangle.
// OGL: maps to a normalized [0,1] quad. SW: computes colfrac/rowfrac to cover the
// full image across w x h pixels regardless of the image's native dimensions.
void Material::DrawStretched(float x, float y, float w, float h)
{
    if (rendermode == render_opengl && oglrenderer && oglrenderer->ReadyToDraw())
    {
        float l      = x / vid.width;
        float r      = (x + w) / vid.width;
        float gl_top = 1.0f - y / vid.height;
        float gl_bot = 1.0f - (y + h) / vid.height;
        oglrenderer->Draw2DGraphic(l, gl_bot, r, gl_top, this);
        return;
    }
    // Fall through to SW path when OGL is not ready (e.g. loading screen).

    // SW: stretch the full image to exactly w x h screen pixels.
    TextureRef &tr = tex[0];
    int ix = (int)x;
    int iy = (int)y;
    int x2 = min(ix + (int)w, vid.width);
    int y2 = min(iy + (int)h, vid.height);
    if (x2 <= ix || y2 <= iy)
        return;

    // Advance one full image across the draw rectangle.
    fixed_t colfrac = (float)tr.t->width  / (float)(x2 - ix);
    fixed_t rowfrac = (float)tr.t->height / (float)(y2 - iy);

    byte *dest_tl = vid.screens[0] + vid.scaledofs + iy * vid.width + ix;
    byte *dest_tr = dest_tl + (x2 - ix);
    byte *dest_bl = dest_tl + (y2 - iy) * vid.width;

    tr.t->Draw(dest_tl, dest_tr, dest_bl, fixed_t(0), fixed_t(0), colfrac, rowfrac, 0);
}

// Draw the top draw_h rows of the image as if it were full_h pixels tall (no stretch).
// The image is placed at (x,y) and scaled to w x full_h, but only the top draw_h rows
// are visible — like sliding a curtain down over a fixed background.
// OGL: draws the full-height quad and uses GL_SCISSOR_TEST to clip the bottom.
// SW: advances rowfrac over full_h rows but writes only draw_h destination rows.
void Material::DrawStretchedTop(float x, float y, float w, float draw_h, float full_h)
{
    if (draw_h <= 0.0f || full_h <= 0.0f)
        return;
    if (draw_h >= full_h)
    {
        DrawStretched(x, y, w, full_h);
        return;
    }

    if (rendermode == render_opengl && oglrenderer && oglrenderer->ReadyToDraw())
    {
        float l      = x / vid.width;
        float r      = (x + w) / vid.width;
        float gl_top = 1.0f - y / vid.height;
        float gl_bot = 1.0f - (y + full_h) / vid.height; // full-height quad bottom

        // Scissor clips output to the top draw_h rows of the screen slot.
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, (int)(vid.height - (y + draw_h)), (int)vid.width, (int)draw_h);
        oglrenderer->Draw2DGraphic(l, gl_bot, r, gl_top, this);
        glDisable(GL_SCISSOR_TEST);
        return;
    }

    // SW: sample only the top draw_h/full_h fraction of the image.
    TextureRef &tr = tex[0];
    int ix = (int)x;
    int iy = (int)y;
    int x2 = min(ix + (int)w,      vid.width);
    int y2 = min(iy + (int)draw_h, vid.height);
    if (x2 <= ix || y2 <= iy)
        return;

    // colfrac: advance one full image width across w columns.
    fixed_t colfrac = (float)tr.t->width  / (float)(x2 - ix);
    // rowfrac: advance one full image height across full_h rows (not draw_h).
    fixed_t rowfrac = (float)tr.t->height / (float)full_h;

    byte *dest_tl = vid.screens[0] + vid.scaledofs + iy * vid.width + ix;
    byte *dest_tr = dest_tl + (x2 - ix);
    byte *dest_bl = dest_tl + (y2 - iy) * vid.width;

    tr.t->Draw(dest_tl, dest_tr, dest_bl, fixed_t(0), fixed_t(0), colfrac, rowfrac, 0);
}

// Draw the image with its bottom edge at bottom_y, h pixels tall. If bottom_y < h the
// top of the image is above the screen and is clipped (image slides in/out from above).
// OGL: the quad extends above the screen; the clip planes handle it automatically.
// SW: off-screen rows are skipped by offsetting into the source image.
void Material::DrawStretchedBottom(float x, float bottom_y, float w, float h)
{
    if (h <= 0.0f)
        return;

    if (rendermode == render_opengl && oglrenderer && oglrenderer->ReadyToDraw())
    {
        float l      = x / vid.width;
        float r      = (x + w) / vid.width;
        float gl_bot = 1.0f - bottom_y / vid.height;
        float gl_top = 1.0f - (bottom_y - h) / vid.height; // may be > 1.0 when top is off-screen
        oglrenderer->Draw2DGraphic(l, gl_bot, r, gl_top, this);
        return;
    }

    // SW: clip the top of the image to the screen edge.
    TextureRef &tr = tex[0];
    int ix       = (int)x;
    int x2       = min(ix + (int)w, vid.width);
    int img_top  = (int)(bottom_y - h);       // top of image in screen coords; may be negative
    int dest_y   = max(0, img_top);           // first visible dest row
    int dest_y2  = min((int)bottom_y, vid.height);
    if (x2 <= ix || dest_y2 <= dest_y)
        return;

    fixed_t colfrac = (float)tr.t->width  / (float)(x2 - ix);
    fixed_t rowfrac = (float)tr.t->height / h;

    // Source row corresponding to the first visible dest row.
    fixed_t row_start = (float)(dest_y - img_top) * (float)tr.t->height / h;

    byte *dest_tl = vid.screens[0] + vid.scaledofs + dest_y * vid.width + ix;
    byte *dest_tr = dest_tl + (x2 - ix);
    byte *dest_bl = dest_tl + (dest_y2 - dest_y) * vid.width;

    tr.t->Draw(dest_tl, dest_tr, dest_bl, fixed_t(0), row_start, colfrac, rowfrac, 0);
}

void Material::DrawFullscreen()
{
    if (rendermode == render_opengl && oglrenderer && oglrenderer->ReadyToDraw())
    {
        oglrenderer->DrawFullscreenGraphic(this, worldwidth, worldheight);
        return;
    }
    // SW fallback: stretch to fill the framebuffer.
    DrawStretched(0, 0, vid.width, vid.height);
}

void Material::DrawFill(int x, int y, int w, int h)
{
    if (rendermode == render_opengl)
    {
        if (oglrenderer && oglrenderer->ReadyToDraw())
            oglrenderer->Draw2DGraphicFill_Doom(x, y, w, h, this);
        return;
    }

    tex[0].t->DrawFill(x, y, w, h);
}

// Fills a box of pixels using a repeated texture as a pattern,
// scaled to screen size.
void LumpTexture::DrawFill(int x, int y, int w, int h)
{
    byte *flat = GetData(); // in col-major order
    byte *base_dest = vid.screens[0] + y * vid.dupy * vid.width + x * vid.dupx + vid.scaledofs;

    w *= vid.dupx;
    h *= vid.dupy;

    fixed_t dx = fixed_t(1) / vid.dupx;
    fixed_t dy = fixed_t(1) / vid.dupy;

    fixed_t xfrac = 0;
    for (int u = 0; u < w; u++, xfrac += dx, base_dest++)
    {
        fixed_t yfrac = 0;
        byte *src = flat + (xfrac.floor() % width) * height;
        byte *dest = base_dest;
        for (int v = 0; v < h; v++, yfrac += dy)
        {
            *dest = src[yfrac.floor() % height];
            dest += vid.width;
        }
    }
}

//=================================================================
//                         Blitting
//=================================================================

/// Copies a rectangular area from one screen buffer to another
void V_CopyRect(float srcx,
                float srcy,
                int srcscrn,
                float width,
                float height,
                float destx,
                float desty,
                int destscrn)
{
    // WARNING don't mix
    if ((srcscrn & V_SLOC) || (destscrn & V_SLOC))
    {
        srcx *= vid.dupx;
        srcy *= vid.dupy;
        width *= vid.dupx;
        height *= vid.dupy;
        destx *= vid.dupx;
        desty *= vid.dupy;
    }
    srcscrn &= 0xffff;
    destscrn &= 0xffff;

#ifdef RANGECHECK
    if (srcx < 0 || srcx + width > vid.width || srcy < 0 || srcy + height > vid.height ||
        destx < 0 || destx + width > vid.width || desty < 0 || desty + height > vid.height ||
        (unsigned)srcscrn > 4 || (unsigned)destscrn > 4)
        I_Error("Bad V_CopyRect %d %d %d %d %d %d %d %d",
                srcx,
                srcy,
                srcscrn,
                width,
                height,
                destx,
                desty,
                destscrn);
#endif

    int sx = int(srcx);
    int sy = int(srcy);
    int dx = int(destx);
    int dy = int(desty);
    int w = int(width);
    int h = int(height);

    byte *src = vid.screens[srcscrn] + vid.width * sy + sx + vid.scaledofs;
    byte *dest = vid.screens[destscrn] + vid.width * dy + dx + vid.scaledofs;

    for (; h > 0; h--)
    {
        memcpy(dest, src, w);
        src += vid.width;
        dest += vid.width;
    }
}

// Copy a rectangular area from one bitmap to another (8bpp)
void VID_BlitLinearScreen(
    byte *src, byte *dest, int width, int height, int srcrowbytes, int destrowbytes)
{
    if (srcrowbytes == destrowbytes)
        memcpy(dest, src, srcrowbytes * height);
    else
    {
        while (height--)
        {
            memcpy(dest, src, width);

            dest += destrowbytes;
            src += srcrowbytes;
        }
    }
}

//======================================================================
//                     Misc. drawing stuff
//======================================================================

//  Fills a box of pixels with a single color, NOTE: scaled to screen size
void V_DrawFill(int x, int y, int w, int h, int c)
{
    if (rendermode != render_soft)
    {
        OGLRenderer::DrawFill(x, y, w, h, c);
        return;
    }

    byte *dest = vid.screens[0] + y * vid.dupy * vid.width + x * vid.dupx + vid.scaledofs;

    w *= vid.dupx;
    h *= vid.dupy;

    for (int v = 0; v < h; v++, dest += vid.width)
        for (int u = 0; u < w; u++)
            dest[u] = c;
}

//
//  Fade all the screen buffer, so that the menu is more readable,
//  especially now that we use the small hufont in the menus...
//
void V_DrawFadeScreen()
{
    if (rendermode != render_soft)
    {
        OGLRenderer::FadeScreenMenuBack(0x01010160, 0); // faB: hack, 0 means full height :o
        return;
    }

    int w = vid.width >> 2;

    byte p1, p2, p3, p4;
    byte *fadetable = R_GetFadetable(0)->colormap + 16 * 256;

    for (int y = 0; y < vid.height; y++)
    {
        int *buf = (int *)(vid.screens[0] + vid.width * y);
        for (int x = 0; x < w; x++)
        {
            int quad = buf[x];
            p1 = fadetable[quad & 255];
            p2 = fadetable[(quad >> 8) & 255];
            p3 = fadetable[(quad >> 16) & 255];
            p4 = fadetable[quad >> 24];
            buf[x] = (p4 << 24) | (p3 << 16) | (p2 << 8) | p1;
        }
    }

#ifdef _16bitcrapneverfinished
    else
    {
        w = vid.width;
        for (y = 0; y < vid.height; y++)
        {
            wput = (short *)(vid.screens[0] + vid.width * y);
            for (x = 0; x < w; x++)
            {
                *wput = (*wput >> 1) & 0x3def;
                wput++;
            }
        }
    }
#endif
}

// Switch to full-screen 2D mode (no HUD aspect-ratio pillarboxing) for console rendering.
// Call before drawing the console background and text.
void V_SetupConsoleDraw()
{
    if (rendermode == render_opengl && oglrenderer && oglrenderer->ReadyToDraw())
        oglrenderer->Setup2DMode_Full();
}

// Restore the standard pillarboxed 2D mode after console rendering so that
// subsequent HUD/menu drawing is properly aspect-corrected.
void V_RestoreHUDDraw()
{
    if (rendermode == render_opengl && oglrenderer && oglrenderer->ReadyToDraw())
        oglrenderer->Setup2DMode();
}

/// Simple translucence with one color, coords are true LFB coords
void V_DrawFadeConsBack(float x1, float y1, float x2, float y2)
{
    if (rendermode != render_soft)
    {
        // FIXME OGLRenderer::FadeScreenMenuBack(0x00500000, y2);
        return;
    }

    // Defensive: check for uninitialized video or console
    if (!vid.screens[0] || !greenmap || vid.width <= 0 || vid.height <= 0)
        return;

    // Clamp coordinates to screen bounds
    if (x1 < 0)
        x1 = 0;
    if (y1 < 0)
        y1 = 0;
    if (x2 > vid.width)
        x2 = vid.width;
    if (y2 > vid.height)
        y2 = vid.height;
    if (x1 >= x2 || y1 >= y2)
        return;

    if (vid.BytesPerPixel == 1)
    {
        for (int y = int(y1); y < int(y2); y++)
        {
            byte *buf = vid.screens[0] + vid.width * y;
            for (int x = int(x1); x < int(x2); x++)
                buf[x] = greenmap[buf[x]];
        }
    }
    else
    {
        int w = int(x2 - x1);
        for (int y = int(y1); y < int(y2); y++)
        {
            short *wput = (short *)(vid.screens[0] + vid.width * y) + int(x1);
            for (int x = 0; x < w; x++)
            {
                *wput = ((*wput & 0x7bde) + (15 << 5)) >> 1;
                wput++;
            }
        }
    }
}
