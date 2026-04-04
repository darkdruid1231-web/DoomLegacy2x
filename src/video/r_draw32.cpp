// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_draw32.cpp
//
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
//
// \file
// \brief 32bpp (ARGB8888) span/column drawer functions
//
// 32-bit software rendering using ARGB8888 format.
// Colormaps store pre-converted ARGB8888 values.
//-----------------------------------------------------------------------------

#include "doomtype.h"

#include "r_draw.h"
#include "r_render.h"

// palette32 - pre-converted ARGB8888 colors for each palette entry
// Initialized by R_Init8to16() from PLAYPAL
extern Uint32 palette32[256];

//==========================================================================
// COLUMNS
//==========================================================================

// Standard column drawer for 32-bit
void R_DrawColumn_32(void)
{
    int count = dc_yh - dc_yl + 1;

    if (count <= 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawColumn_32: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    Uint32 *dest = (Uint32 *)(ylookup[dc_yl] + columnofs[dc_x]);

    fixed_t fracstep = dc_iscale;
    fixed_t frac = dc_texturemid + (dc_yl - centery) * fracstep;

    {
        register const byte *source = dc_source;
        register const byte *colormap = dc_colormap;
        register int heightmask = dc_texheight - 1;

        if (dc_texheight & heightmask)
        {
            heightmask++;
            fixed_t fheightmask = heightmask;

            if (frac < 0)
                while ((frac += fheightmask) < 0)
                    ;
            else
                while (frac >= fheightmask)
                    frac -= fheightmask;

            do
            {
                *dest = palette32[colormap[source[frac.floor()]]];
                dest += vid.width;
                if ((frac += fracstep) >= fheightmask)
                    frac -= fheightmask;
            } while (--count);
        }
        else
        {
            while ((count -= 2) >= 0)
            {
                *dest = palette32[colormap[source[frac.floor() & heightmask]]];
                dest += vid.width;
                frac += fracstep;
                *dest = palette32[colormap[source[frac.floor() & heightmask]]];
                dest += vid.width;
                frac += fracstep;
            }
            if (count & 1)
                *dest = palette32[colormap[source[frac.floor() & heightmask]]];
        }
    }
}

// 32-bit version of sky column drawer
void R_DrawSkyColumn_32(void)
{
    int count = dc_yh - dc_yl + 1;

    if (count <= 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawColumn_32: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    Uint32 *dest = (Uint32 *)(ylookup[dc_yl] + columnofs[dc_x]);

    fixed_t fracstep = dc_iscale;
    fixed_t frac = dc_texturemid + (dc_yl - centery) * fracstep;

    {
        register const byte *source = dc_source;
        register const byte *colormap = dc_colormap;

        while ((count -= 2) >= 0)
        {
            *dest = palette32[colormap[source[frac.floor() & 255]]];
            dest += vid.width;
            frac += fracstep;
            *dest = palette32[colormap[source[frac.floor() & 255]]];
            dest += vid.width;
            frac += fracstep;
        }
        if (count & 1)
            *dest = palette32[colormap[source[frac.floor() & 255]]];
    }
}

// The standard Doom 'fuzzy' (blur, shadow) effect for 32-bit
void R_DrawFuzzColumn_32()
{
    // Adjust borders. Low...
    if (!dc_yl)
        dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    int count = dc_yh - dc_yl;

    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawFuzzColumn_32: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    Uint32 *dest = (Uint32 *)(ylookup[dc_yl] + columnofs[dc_x]);

    do
    {
        // Lookup framebuffer, and retrieve a pixel that is either one column
        // left or right of the current one. Apply fuzz by darkening.
        Uint32 shifted_pixel = ((Uint32 *)dest)[fuzzoffset[fuzzpos]];
        // Simple fuzz: darken the pixel
        Uint32 a = shifted_pixel & 0xFF000000;
        Uint32 r = (shifted_pixel >> 16) & 0xFF;
        Uint32 g = (shifted_pixel >> 8) & 0xFF;
        Uint32 b = shifted_pixel & 0xFF;
        r = (r * 6) >> 4;  // darken more for fuzz effect
        g = (g * 6) >> 4;
        b = (b * 6) >> 4;
        *dest = a | (r << 16) | (g << 8) | b;

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;

        dest = (Uint32 *)((byte *)dest + vid.width * sizeof(Uint32));
    } while (count--);
}

// Translucent column for 32-bit
void R_DrawTranslucentColumn_32(void)
{
    int count = dc_yh - dc_yl + 1;

    if (count <= 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawTranslucentColumn_32: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    Uint32 *dest = (Uint32 *)(ylookup[dc_yl] + columnofs[dc_x]);

    fixed_t fracstep = dc_iscale;
    fixed_t frac = dc_texturemid + (dc_yl - centery) * fracstep;

    {
        register const byte *source = dc_source;
        register const byte *colormap = dc_colormap;
        register int heightmask = dc_texheight - 1;

        if (dc_texheight & heightmask)
        {
            heightmask++;
            fixed_t fheightmask = heightmask;

            if (frac < 0)
                while ((frac += fheightmask) < 0)
                    ;
            else
                while (frac >= fheightmask)
                    frac -= fheightmask;

            do
            {
                Uint32 src_col = palette32[colormap[source[frac.floor()]]];
                Uint32 dst_col = *dest;
                // 50/50 blend
                Uint32 a = ((src_col >> 24) + (dst_col >> 24)) >> 1;
                Uint32 r = (((src_col >> 16) & 0xFF) + ((dst_col >> 16) & 0xFF)) >> 1;
                Uint32 g = (((src_col >> 8) & 0xFF) + ((dst_col >> 8) & 0xFF)) >> 1;
                Uint32 b = ((src_col & 0xFF) + (dst_col & 0xFF)) >> 1;
                *dest = (a << 24) | (r << 16) | (g << 8) | b;

                dest = (Uint32 *)((byte *)dest + vid.width * sizeof(Uint32));
                if ((frac += fracstep) >= fheightmask)
                    frac -= fheightmask;
            } while (--count);
        }
        else
        {
            while ((count -= 2) >= 0)
            {
                Uint32 src_col = palette32[colormap[source[frac.floor() & heightmask]]];
                Uint32 dst_col = *dest;
                Uint32 a = ((src_col >> 24) + (dst_col >> 24)) >> 1;
                Uint32 r = (((src_col >> 16) & 0xFF) + ((dst_col >> 16) & 0xFF)) >> 1;
                Uint32 g = (((src_col >> 8) & 0xFF) + ((dst_col >> 8) & 0xFF)) >> 1;
                Uint32 b = ((src_col & 0xFF) + (dst_col & 0xFF)) >> 1;
                *dest = (a << 24) | (r << 16) | (g << 8) | b;
                dest = (Uint32 *)((byte *)dest + vid.width * sizeof(Uint32));
                frac += fracstep;

                src_col = palette32[colormap[source[frac.floor() & heightmask]]];
                dst_col = *dest;
                a = ((src_col >> 24) + (dst_col >> 24)) >> 1;
                r = (((src_col >> 16) & 0xFF) + ((dst_col >> 16) & 0xFF)) >> 1;
                g = (((src_col >> 8) & 0xFF) + ((dst_col >> 8) & 0xFF)) >> 1;
                b = ((src_col & 0xFF) + (dst_col & 0xFF)) >> 1;
                *dest = (a << 24) | (r << 16) | (g << 8) | b;
                dest = (Uint32 *)((byte *)dest + vid.width * sizeof(Uint32));
                frac += fracstep;
            }
            if (count & 1)
            {
                Uint32 src_col = palette32[colormap[source[frac.floor() & heightmask]]];
                Uint32 dst_col = *dest;
                Uint32 a = ((src_col >> 24) + (dst_col >> 24)) >> 1;
                Uint32 r = (((src_col >> 16) & 0xFF) + ((dst_col >> 16) & 0xFF)) >> 1;
                Uint32 g = (((src_col >> 8) & 0xFF) + ((dst_col >> 8) & 0xFF)) >> 1;
                Uint32 b = ((src_col & 0xFF) + (dst_col & 0xFF)) >> 1;
                *dest = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }
}

// Draw columns with remapped green ramp for player sprites (32-bit)
void R_DrawTranslatedColumn_32(void)
{
    int count = dc_yh - dc_yl;

    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawTranslatedColumn_32: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    Uint32 *dest = (Uint32 *)(ylookup[dc_yl] + columnofs[dc_x]);

    fixed_t fracstep = dc_iscale;
    fixed_t frac = dc_texturemid + (dc_yl - centery) * fracstep;

    {
        register const byte *colormap = dc_colormap;

        do
        {
            *dest = palette32[colormap[dc_translation[dc_source[frac.floor()]]]];
            dest = (Uint32 *)((byte *)dest + vid.width * sizeof(Uint32));
            frac += fracstep;
        } while (count--);
    }
}

// Shade column for 32-bit
void R_DrawShadeColumn_32()
{
    if ((dc_yl < 0) || (dc_x >= vid.width))
        return;

    int count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawShadeColumn_32: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    Uint32 *dest = (Uint32 *)(ylookup[dc_yl] + columnofs[dc_x]);

    fixed_t fracstep = dc_iscale;
    fixed_t frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        Uint32 src_col = palette32[dc_colormap[dc_source[frac.floor()]]];
        // Shade: blend with black (50/50)
        Uint32 r = (src_col >> 16) & 0xFF;
        Uint32 g = (src_col >> 8) & 0xFF;
        Uint32 b = src_col & 0xFF;
        r >>= 1;
        g >>= 1;
        b >>= 1;
        *dest = (src_col & 0xFF000000) | (r << 16) | (g << 8) | b;

        dest = (Uint32 *)((byte *)dest + vid.width * sizeof(Uint32));
        frac += fracstep;
    } while (count--);
}

// Fog column for 32-bit
void R_DrawFogColumn_32()
{
    int count = dc_yh - dc_yl;

    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawFogColumn_32: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    Uint32 *dest = (Uint32 *)(ylookup[dc_yl] + columnofs[dc_x]);

    register const byte *colormap = dc_colormap;

    do
    {
        // Simple. Apply the colormap to what's already on the screen.
        *dest = palette32[colormap[*dest & 0xFF]];
        dest = (Uint32 *)((byte *)dest + vid.width * sizeof(Uint32));
    } while (count--);
}

// Column shadowed for 3D floors (32-bit)
void R_DrawColumnShadowed_32()
{
    int realyh = dc_yh;
    int realyl = dc_yl;

    int count = dc_yh - dc_yl;

    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawColumnShadowed_32: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    int bheight = 0;

    // Run through the lightlist from top to bottom and cut up the column.
    for (int i = 0; i < dc_numlights; i++)
    {
        int solid = dc_lightlist[i].flags & FF_CUTSOLIDS;

        int height = dc_lightlist[i].height.floor();
        if (solid)
            bheight = dc_lightlist[i].botheight.floor();
        if (height <= dc_yl)
        {
            dc_colormap = dc_lightlist[i].rcolormap;
            if (solid && dc_yl < bheight)
                dc_yl = bheight;
            continue;
        }
        dc_yh = height;

        if (dc_yh > realyh)
            dc_yh = realyh;
        R_DrawColumn_32();
        if (solid)
            dc_yl = bheight;
        else
            dc_yl = dc_yh + 1;

        dc_colormap = dc_lightlist[i].rcolormap;
    }
    dc_yh = realyh;
    if (dc_yl <= realyh)
        R_DrawColumn_32();
}

//==========================================================================
// SPANS
//==========================================================================

// Standard span drawer for 32-bit
void R_DrawSpan_32()
{
#ifdef RANGECHECK
    if (ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= vid.width || (unsigned)ds_y > vid.height)
        I_Error("R_DrawSpan_32: %i to %i at %i", ds_x1, ds_x2, ds_y);
#endif

    Uint32 xfrac = ds_xfrac.value();
    Uint32 yfrac = ds_yfrac.value();

    Uint32 *dest = (Uint32 *)(ylookup[ds_y] + columnofs[ds_x1]);

    int count = ds_x2 - ds_x1 + 1;

    Uint32 xmask = ((1 << ds_xbits) - 1) << 16;
    Uint32 ymask = (1 << ds_ybits) - 1;
    int xshift = 16 - ds_ybits;

    const byte *colormap = ds_colormap;

    while (count)
    {
        int spot = ((xfrac & xmask) >> xshift) | (yfrac & ymask);
        *dest++ = palette32[colormap[ds_source[spot]]];
        xfrac += ds_xstep.value();
        yfrac += ds_ystep.value();
        count--;
    }
}

// Translucent span for 32-bit
void R_DrawTranslucentSpan_32()
{
#ifdef RANGECHECK
    if (ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= vid.width || (unsigned)ds_y > vid.height)
        I_Error("R_DrawTranslucentSpan_32: %i to %i at %i", ds_x1, ds_x2, ds_y);
#endif

    Uint32 xfrac = ds_xfrac.value();
    Uint32 yfrac = ds_yfrac.value();

    Uint32 *dest = (Uint32 *)(ylookup[ds_y] + columnofs[ds_x1]);

    int count = ds_x2 - ds_x1 + 1;

    Uint32 xmask = ((1 << ds_xbits) - 1) << 16;
    Uint32 ymask = (1 << ds_ybits) - 1;
    int xshift = 16 - ds_ybits;

    const byte *colormap = ds_colormap;
    const byte *transmap = ds_transmap;

    while (count)
    {
        int spot = ((xfrac & xmask) >> xshift) | (yfrac & ymask);
        Uint32 src_col = palette32[colormap[ds_source[spot]]];
        Uint32 dst_col = *dest;
        // 50/50 blend
        Uint32 a = ((src_col >> 24) + (dst_col >> 24)) >> 1;
        Uint32 r = (((src_col >> 16) & 0xFF) + ((dst_col >> 16) & 0xFF)) >> 1;
        Uint32 g = (((src_col >> 8) & 0xFF) + ((dst_col >> 8) & 0xFF)) >> 1;
        Uint32 b = ((src_col & 0xFF) + (dst_col & 0xFF)) >> 1;
        *dest = (a << 24) | (r << 16) | (g << 8) | b;

        dest++;
        xfrac += ds_xstep.value();
        yfrac += ds_ystep.value();
        count--;
    }
}

// Fog span for 32-bit
void R_DrawFogSpan_32()
{
    Uint32 *dest = (Uint32 *)(ylookup[ds_y] + columnofs[ds_x1]);
    unsigned count = ds_x2 - ds_x1 + 1;

    const byte *colormap = ds_colormap;

    while (count >= 4)
    {
        dest[0] = palette32[colormap[dest[0] & 0xFF]];
        dest[1] = palette32[colormap[dest[1] & 0xFF]];
        dest[2] = palette32[colormap[dest[2] & 0xFF]];
        dest[3] = palette32[colormap[dest[3] & 0xFF]];

        dest += 4;
        count -= 4;
    }

    while (count--)
    {
        *dest = palette32[colormap[*dest & 0xFF]];
        dest++;
    }
}
