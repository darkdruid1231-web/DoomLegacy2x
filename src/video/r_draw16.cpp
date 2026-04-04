// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_draw16.cpp 310 2006-02-11 17:14:10Z jussip $
//
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief 16bpp (HIGHCOLOR) span/column drawer functions
//
//  NOTE: no includes because this is included as part of r_draw.c
//
// THIS IS THE VERY BEGINNING OF DOOM LEGACY's HICOLOR MODE (or TRUECOLOR)
// Doom Legacy will use HICOLOR textures, rendered through software, and
// may use MMX, or WILL use MMX to get a decent speed.
//

#include "doomtype.h"

#include "r_draw.h"
#include "r_render.h"

// ==========================================================================
// COLUMNS
// ==========================================================================

// r_data.c
extern short color8to16[256];     // remap color index to 15-bit highcolor
extern short color8to16_565[256];  // remap color index to RGB565 16-bit color
extern short *hicolormaps;         // colormap for high->high color remapping

//  standard upto 128high posts column drawer
//
void R_DrawColumn_16(void)
{
    int count;
    short *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl + 1;

    // Zero length, column does not exceed a pixel.
    if (count <= 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows?
    dest = (short *)(ylookup[dc_yl] + columnofs[dc_x]);

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.

    do
    {
        // Re-map color indices from wall texture column
        //  using a lighting/special effects LUT, then convert to RGB565.
        *dest = color8to16_565[dc_colormap[dc_source[(frac.floor()) & 127]]];

        dest += vid.width;
        frac += fracstep;

    } while (--count);
}

//  LAME cutnpaste : same as R_DrawColumn_16 but wraps around 256
//  instead of 128 for the tall sky textures (256x240)
//
void R_DrawSkyColumn_16(void)
{
    int count;
    short *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl + 1;

    // Zero length, column does not exceed a pixel.
    if (count <= 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    dest = (short *)(ylookup[dc_yl] + columnofs[dc_x]);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        // Sky texture - use colormap to get palette index, then convert to RGB565
        *dest = color8to16_565[dc_colormap[dc_source[(frac.floor()) & 255]]];

        dest += vid.width;
        frac += fracstep;

    } while (--count);
}

//
//
// #ifndef USEASM
void R_DrawFuzzColumn_16()
{
    short *dest;
    fixed_t frac;
    fixed_t fracstep;

    // Adjust borders. Low...
    if (!dc_yl)
        dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    int count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
    {
        I_Error("R_DrawFuzzColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }
#endif

    // Does not work with blocky mode.
    dest = (short *)(ylookup[dc_yl] + columnofs[dc_x]);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        // Lookup framebuffer pixel, and retrieve a pixel that is either
        // one column left or right of the current one. Apply fuzz colormap.
        // dest is short* (RGB565), read the shifted pixel as a byte index approximation.
        byte shifted_pixel = ((byte *)dest)[fuzzoffset[fuzzpos]];
        *dest = color8to16_565[R.base_colormap[6 * 256 + shifted_pixel]];

        // Clamp table lookup index.
        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;

        dest += vid.width;

        frac += fracstep;
    } while (count--);
}
// #endif

//
//
// #ifndef USEASM
void R_DrawTranslucentColumn_16(void)
{
    int count;
    short *dest;
    fixed_t frac;
    fixed_t fracstep;

    // byte*               src;

    // check out coords for src*
    if ((dc_yl < 0) || (dc_x >= vid.width))
        return;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
    {
        I_Error("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }

#endif

    // FIXME. As above.
    // src  = ylookup[dc_yl] + columnofs[dc_x+2];
    dest = (short *)(ylookup[dc_yl] + columnofs[dc_x]);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    // Here we do an additional index re-mapping.
    do
    {
        // RGB565 translucency: blend source and dest 50/50
        unsigned short src = color8to16_565[dc_source[frac.floor()]];
        unsigned short dst = *dest;
        unsigned src_r = (src >> 11) & 0x1F;
        unsigned src_g = (src >> 5) & 0x3F;
        unsigned src_b = src & 0x1F;
        unsigned dst_r = (dst >> 11) & 0x1F;
        unsigned dst_g = (dst >> 5) & 0x3F;
        unsigned dst_b = dst & 0x1F;
        // 50/50 blend
        unsigned r = (src_r + dst_r) >> 1;
        unsigned g = (src_g + dst_g) >> 1;
        unsigned b = (src_b + dst_b) >> 1;
        *dest = (r << 11) | (g << 5) | b;

        dest += vid.width;
        frac += fracstep;
    } while (count--);
}
// #endif

//
//
// #ifndef USEASM
void R_DrawTranslatedColumn_16(void)
{
    int count;
    short *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
    {
        I_Error("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }

#endif

    dest = (short *)(ylookup[dc_yl] + columnofs[dc_x]);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    // Here we do an additional index re-mapping.
    do
    {
        *dest = color8to16_565[dc_colormap[dc_translation[dc_source[frac.floor()]]]];
        dest += vid.width;

        frac += fracstep;
    } while (count--);
}
// #endif

// Shade column for 16-bit RGB565
void R_DrawShadeColumn_16()
{
    if ((dc_yl < 0) || (dc_x >= vid.width))
        return;

    int count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= vid.width || dc_yl < 0 || dc_yh >= vid.height)
        I_Error("R_DrawShadeColumn_16: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    short *dest = (short *)(ylookup[dc_yl] + columnofs[dc_x]);

    fixed_t fracstep = dc_iscale;
    fixed_t frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        unsigned short src = color8to16_565[dc_colormap[dc_source[frac.floor()]]];
        // Shade: darken by 50%
        unsigned r = (src >> 11) & 0x1F;
        unsigned g = (src >> 5) & 0x3F;
        unsigned b = src & 0x1F;
        r >>= 1;
        g >>= 1;
        b >>= 1;
        *dest = (r << 11) | (g << 5) | b;

        dest += vid.width;
        frac += fracstep;
    } while (count--);
}

// ==========================================================================
// SPANS
// ==========================================================================

//
//
// #ifndef USEASM
void R_DrawSpan_16(void)
{
    Sint32 xfrac;
    Sint32 yfrac;
    short *dest;
    int count;
    int spot;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= vid.width || (unsigned)ds_y > vid.height)
    {
        I_Error("R_DrawSpan: %i to %i at %i", ds_x1, ds_x2, ds_y);
    }
#endif

    xfrac = ds_xfrac.value();
    yfrac = ds_yfrac.value();

    dest = (short *)(ylookup[ds_y] + columnofs[ds_x1]);

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

    do
    {
        // Current texture index in u,v.
        spot = ((yfrac >> (16 - 6)) & (63 * 64)) + ((xfrac >> 16) & 63);

        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap, then convert to RGB565.
        *dest++ = color8to16_565[ds_colormap[ds_source[spot]]];

        // Next step in u,v.
        xfrac += ds_xstep.value();
        yfrac += ds_ystep.value();

    } while (count--);
}
// #endif
