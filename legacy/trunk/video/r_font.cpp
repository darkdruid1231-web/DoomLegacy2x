// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1998-2009 by DooM Legacy Team.
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
/// \brief Font system.

#include "console.h"
#include "g_game.h"

#include "i_video.h"
#include "r_data.h"
#include "screen.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

// SDL_ttf is enabled via CMake when HAVE_TTF is defined
#ifndef HAVE_TTF
#define NO_TTF
#endif

/// ucs2 because SDL_ttf currently only understands Unicode codepoints within the BMP.
static int utf8_to_ucs2(const char *utf8, Uint16 *ucs2)
{
    int ch = *(const byte *)utf8;

    // TODO check that in multibyte seqs bytes other than first start with bits 10, otherwise give
    // error

    if (ch < 0x80)
    {
        *ucs2 = ch;
        return 1;
    }

    int n = strlen(utf8);

    if (ch < 0xe0) // >= 0xc0)
    {
        if (n < 2)
            return 0;
        ch = int(utf8[0] & 0x1f) << 6;
        ch |= int(utf8[1] & 0x3f);
        *ucs2 = ch;
        return 2;
    }
    else if (ch < 0xf0)
    {
        if (n < 3)
            return 0;
        ch = int(utf8[0] & 0x0f) << 12;
        ch |= int(utf8[1] & 0x3f) << 6;
        ch |= int(utf8[2] & 0x3f);
        *ucs2 = ch;
        return 3;
    }
    else
    {
        if (n < 4)
            return 0;
        ch = int(utf8[0] & 0x07) << 18;
        ch |= int(utf8[1] & 0x3f) << 12;
        ch |= int(utf8[2] & 0x3f) << 6;
        ch |= int(utf8[3] & 0x3f);
        *ucs2 = ch;
        return 4;
    }
}

#define HU_FONTSTART '!'                            // the first font character
#define HU_FONTEND '_'                              // the last font character
#define HU_FONTSIZE (HU_FONTEND - HU_FONTSTART + 1) // default font size

// Doom:
// STCFN033-95 + 121 : small red font

// Heretic:
// FONTA01-59 : medium silver font
// FONTB01-58 : large green font, some symbols empty

// Hexen:
// FONTA01-59 : medium silver font
// FONTAY01-59 : like FONTA but yellow
// FONTB01-58 : large red font, some symbols empty

font_t *hud_font;
font_t *big_font;

font_t::~font_t()
{
}

//======================================================================
//                          RASTER FONTS
//======================================================================

/// \brief Doom-style raster font
class rasterfont_t : public font_t
{
  protected:
    char start, end;                    ///< first and last ASCII characters included in the font
    std::vector<class Material *> font; ///< one Material per symbol

  public:
    rasterfont_t(int startlump, int endlump, char firstchar = '!');
    virtual ~rasterfont_t();

    virtual float DrawCharacter(float x, float y, int c, int flags);
    virtual float StringWidth(const char *str);
    virtual float StringWidth(const char *str, int n);
};

rasterfont_t::rasterfont_t(int startlump, int endlump, char firstchar)
{
    if (startlump < 0 || endlump < 0)
        I_Error("Incomplete font!\n");

    int truesize = endlump - startlump + 1; // we have this many lumps
    char lastchar = firstchar + truesize - 1;

    // the font range must include '!' and '_'. We will duplicate letters if need be.
    start = min(firstchar, '!');
    end = max(lastchar, '_');
    int size = end - start + 1;

    font.resize(size);

    for (int i = start; i <= end; i++)
        if (i < firstchar || i > lastchar)
            // replace the missing letters with the first char
            font[i - start] = materials.GetLumpnum(startlump);
        else
            font[i - start] = materials.GetLumpnum(i - firstchar + startlump);

    // use the character '0' as a "prototype" for the font
    if (start <= '0' && '0' <= end)
    {
        lineskip = font['0' - start]->worldheight + 1;
        advance = font['0' - start]->worldwidth;
    }
    else
    {
        lineskip = font[0]->worldheight + 1;
        advance = font[0]->worldwidth;
    }
}

rasterfont_t::~rasterfont_t()
{
    for (int i = start; i <= end; i++)
        font[i - start]->Release();

    font.clear();
}

float rasterfont_t::DrawCharacter(float x, float y, int c, int flags)
{
    if (flags & V_WHITEMAP)
    {
        current_colormap = whitemap;
        flags |= V_MAP;
    }

    c = toupper(c & 0x7f); // just ASCII here
    if (c < start || c > end)
        return 4; // render a little space for unknown chars in DrawString

    Material *m = font[c - start];
    // Defensive: check for null material (font not fully loaded)
    if (!m)
        return 4;

    m->Draw(x, y, flags);

    return m->worldwidth;
}

//  Draw a string using the font
//  NOTE: the text is centered for screens larger than the base width
float font_t::DrawString(float x, float y, const char *str, int flags)
{
    float dupx = 1;
    float dupy = 1;

    if (rendermode == render_opengl)
    {
        const bool base_coords = (flags & V_SLOC) && (flags & V_SSIZE);

        if (flags & V_SSIZE)
        {
            // When drawing in Doom base coordinates, keep advances unscaled.
            dupx = base_coords ? 1.0f : vid.fdupx;
            dupy = base_coords ? 1.0f : vid.fdupy;
        }

        if ((flags & V_SLOC) && !base_coords)
        {
            x *= vid.fdupx;
            y *= vid.fdupy;
            flags &= ~V_SLOC; // not passed on to Texture::Draw
        }
    }
    else
    {
        if (flags & V_SSIZE)
        {
            dupx = vid.dupx;
            dupy = vid.dupy;
        }

        if (flags & V_SLOC)
        {
            x *= vid.dupx;
            y *= vid.dupy;
            flags &= ~V_SLOC; // not passed on to Texture::Draw
        }
    }

    // cursor coordinates
    float cx = x;
    float cy = y;
    float rowheight = lineskip * dupy;

    while (*str)
    {
        Uint16 c;
        int skip = utf8_to_ucs2(str, &c);
        if (!skip) // bad ucs-8 string
            break;

        str += skip;

        if (c == '\n')
        {
            cx = x;
            cy += rowheight;
            continue;
        }

        cx += dupx * DrawCharacter(cx, cy, c, flags);
    }

    return cx - x;
}

// returns the width of the string (unscaled)
float rasterfont_t::StringWidth(const char *str)
{
    float w = 0;

    while (*str)
    {
        Uint16 c;
        int skip = utf8_to_ucs2(str, &c);
        if (!skip) // bad ucs-8 string
            return 0;

        str += skip;

        c = toupper(c & 0x7f); // just ASCII
        if (c < start || c > end)
            w += 4;
        else
            w += font[c - start]->worldwidth;
    }

    return w;
}

// returns the width of the next n chars of str
float rasterfont_t::StringWidth(const char *str, int n)
{
    float w = 0;

    for (; *str && n > 0; n--)
    {
        Uint16 c;
        int skip = utf8_to_ucs2(str, &c);
        if (!skip) // bad ucs-8 string
            return 0;

        str += skip;

        c = toupper(c & 0x7f); // just ASCII
        if (c < start || c > end)
            w += 4;
        else
            w += font[c - start]->worldwidth;
    }

    return w;
}

//======================================================================
//                         TRUETYPE FONTS
//======================================================================

#ifndef NO_TTF
#include <ft2build.h>
#include FT_FREETYPE_H

// Render FreeType glyphs at 2x display size so GL has enough source pixels for
// smooth LINEAR magnification. SCALE halves the world size back to display size.
#define SCALE 2.0f

/// FreeType-based RGBA texture (owns the pixel buffer)
class FTTexture : public LumpTexture
{
    byte *pixels_rgba; // owned, allocated with new byte[]

    virtual void GLGetData() {}

  public:
    FTTexture(const char *n, int w, int h, byte *rgba)
        : LumpTexture(n, -1, w, h), pixels_rgba(rgba) {}

    virtual ~FTTexture()
    {
        delete[] pixels_rgba;
        pixels_rgba = NULL;
    }

    virtual byte *GetData() { return pixels_rgba; }

    virtual GLuint GLPrepare()
    {
        // Upload if not yet uploaded or if ClearGLTextures() reset gl_id to NOTEXTURE
        // after a GL context recreation. pixels_rgba is kept alive for re-upload.
        if (gl_id == NOTEXTURE)
        {
            glGenTextures(1, &gl_id);
            glBindTexture(GL_TEXTURE_2D, gl_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, pixels_rgba);
        }
        return gl_id;
    }

    /// Software renderer: composite RGBA glyph onto screen.
    /// The glyph alpha is the coverage value. Pixel color is fixed red (palette 179).
    virtual void Draw(byte *dest_tl,
                      byte *dest_tr,
                      byte *dest_bl,
                      fixed_t col,
                      fixed_t startrow,
                      fixed_t colfrac,
                      fixed_t rowfrac,
                      int flags) override;
};

// FTTexture::Draw — composites RGBA glyph onto the software renderer framebuffer.
// The glyph's R channel is fixed at 179 (palette index for STCFN average red).
// The A channel is the FreeType coverage (0-255). G and B are 0.
void FTTexture::Draw(byte *dest_tl,
                     byte *dest_tr,
                     byte *dest_bl,
                     fixed_t col,
                     fixed_t startrow,
                     fixed_t colfrac,
                     fixed_t rowfrac,
                     int flags)
{
    byte *base = pixels_rgba; // row-major RGBA

    // 8-bit: indexed palette. 16-bit: RGB565.
    bool is16bit = (vid.BytesPerPixel > 1);

    if (is16bit)
    {
        // RGB565: 5 bits R, 6 bits G, 5 bits B.
        // Precompute the palette-179 color in RGB565.
        // Palette[179] ≈ (179*64/256, 0, 179*32/256) for standard STCFN.
        // We treat R=179, G=0, B=0 → RGB565: R=(179>>3)=22, G=0, B=0 → 0xB000.
        const Uint16 src16 = 0xB000; // solid red in RGB565

        for (; dest_tl < dest_tr; col += colfrac, dest_tl++, dest_bl++)
        {
            const byte *source = base + ((int)col.floor() * height) * 4;
            fixed_t row = startrow;
            for (byte *dest = dest_tl; dest < dest_bl; row += rowfrac, dest += vid.width)
            {
                Uint8 alpha = source[(int)row.floor() * 4 + 3];
                if (alpha < 128)
                    continue; // transparent
                if (alpha >= 128)
                    *((Uint16 *)dest) = src16;
            }
        }
    }
    else
    {
        // 8-bit palette: use palette index 179 (TTF glyphs are red).
        // TTF glyphs: R=179 (palette index), G=0, B=0, A=coverage.
        // We interpret A as coverage. For opaque pixels (A>=128), write 179.
        const byte idx = 179;

        for (; dest_tl < dest_tr; col += colfrac, dest_tl++, dest_bl++)
        {
            const byte *source = base + ((int)col.floor() * height) * 4;
            fixed_t row = startrow;
            for (byte *dest = dest_tl; dest < dest_bl; row += rowfrac, dest += vid.width)
            {
                Uint8 alpha = source[(int)row.floor() * 4 + 3];
                if (alpha < 128)
                    continue; // transparent
                // Apply V_TL translucency if set (approximate for TTF fonts)
                if (flags & V_TL)
                    *dest = transtables[0][(idx << 8) | *dest];
                else
                    *dest = idx;
            }
        }
    }
}

/// \brief TrueType font using FreeType directly
class ttfont_t : public font_t
{
  protected:
    byte *data; ///< Raw contents of the .ttf file.

    int ascent; ///< max ascent of all glyphs in the font in texels

    struct glyph_t
    {
        int minx, maxx, miny, maxy, advance;
        Material *mat;

        glyph_t() : mat(NULL) {}

        ~glyph_t()
        {
            if (mat)
            {
                delete mat->tex[0].t;
                mat->tex.clear();
                delete mat;
                mat = NULL;
            }
        }
    };

    glyph_t glyphcache[256]; ///< Prerendered glyphs for Latin-1 chars to improve efficiency.

    bool BuildGlyph(glyph_t &g, int c);

  public:
    FT_Library ft_library;
    FT_Face    ft_face;

    ttfont_t(int lump)
    {
        data = static_cast<byte *>(fc.CacheLumpNum(lump, PU_DAVE));

        if (FT_Init_FreeType(&ft_library))
            I_Error("FT_Init_FreeType failed\n");
        if (FT_New_Memory_Face(ft_library, (const FT_Byte *)data,
                               (FT_Long)fc.LumpLength(lump), 0, &ft_face))
            I_Error("FT_New_Memory_Face failed\n");

        // Explicitly select Unicode charmap so FT_Load_Char uses Unicode codepoints.
        // Without this, FreeType may pick Symbol or Mac Roman encoding and map
        // ASCII codes to wrong glyphs (e.g. 'D' renders as 'T').
        if (FT_Select_Charmap(ft_face, FT_ENCODING_UNICODE) != 0)
        {
            // No Unicode charmap — log which encodings are available.
            CONS_Printf("FreeType: no Unicode charmap in TTFCONSL font (%d charmaps):\n",
                        ft_face->num_charmaps);
            for (int i = 0; i < ft_face->num_charmaps; i++)
                CONS_Printf("  charmap %d: platform %d encoding %d\n", i,
                            ft_face->charmaps[i]->platform_id,
                            ft_face->charmaps[i]->encoding_id);
        }

        FT_Set_Pixel_Sizes(ft_face, 0, 16); // render at 2x; SCALE=2 halves world size → 8px display

        ascent   = ft_face->size->metrics.ascender >> 6;
        lineskip = (ft_face->size->metrics.height  >> 6) / SCALE;

        FT_Load_Char(ft_face, '0', FT_LOAD_DEFAULT);
        advance = (ft_face->glyph->advance.x >> 6) / SCALE;

        for (int k = 0; k < 256; k++)
            glyphcache[k].mat = NULL;
    }

    virtual ~ttfont_t()
    {
        FT_Done_Face(ft_face);       ft_face    = NULL;
        FT_Done_FreeType(ft_library); ft_library = NULL;
        Z_Free(data);                data       = NULL;
    }

    virtual float StringWidth(const char *str)
    {
        int total = 0;
        while (*str)
        {
            Uint16 c;
            int skip = utf8_to_ucs2(str, &c);
            if (!skip) break;
            str += skip;
            if (FT_Load_Char(ft_face, c, FT_LOAD_DEFAULT) == 0)
                total += ft_face->glyph->advance.x >> 6;
        }
        return total / SCALE;
    }

    virtual float StringWidth(const char *str, int n)
    {
        int total = 0;
        while (*str && n > 0)
        {
            Uint16 c;
            int skip = utf8_to_ucs2(str, &c);
            if (!skip) break;
            str += skip;
            n--;
            if (FT_Load_Char(ft_face, c, FT_LOAD_DEFAULT) == 0)
                total += ft_face->glyph->advance.x >> 6;
        }
        return total / SCALE;
    }

    virtual float DrawCharacter(float x, float y, int c, int flags);
};

bool ttfont_t::BuildGlyph(glyph_t &g, int c)
{
    if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER))
        return false;

    FT_GlyphSlot slot = ft_face->glyph;
    FT_Bitmap   &bm   = slot->bitmap;

    g.advance = slot->advance.x >> 6;
    g.minx    = slot->bitmap_left;
    g.maxy    = slot->bitmap_top;
    g.miny    = g.maxy - (int)bm.rows;
    g.maxx    = g.minx + (int)bm.width;

    int w = (bm.width > 0) ? (int)bm.width : 1;
    int h = (bm.rows  > 0) ? (int)bm.rows  : 1;

    // Convert FT_PIXEL_MODE_GRAY coverage bytes to red+alpha RGBA.
    // Red matches the Doom raster console font (STCFN* patches are red pixels).
    // Draw2DGraphic hardcodes glColor4f(1,1,1,1) so GL color cannot tint;
    // the pixel data must carry the desired color.
    byte *rgba = new byte[w * h * 4]();
    if (bm.buffer)
    {
        for (int row = 0; row < (int)bm.rows; row++)
        {
            const byte *src = bm.buffer + row * bm.pitch;
            byte       *dst = rgba      + row * w * 4;
            for (int col = 0; col < (int)bm.width; col++)
            {
                dst[col*4 + 0] = 179; // R — matches STCFN raster font average (~palette index 182)
                dst[col*4 + 1] = 0;   // G
                dst[col*4 + 2] = 0;   // B
                dst[col*4 + 3] = src[col]; // A = coverage
            }
        }
    }

    string name("glyph ");
    name += (char)c;

    Texture *t = new FTTexture(name.c_str(), w, h, rgba);
    // topoffs: pixels the glyph top sits ABOVE the draw origin.
    // For baseline alignment: glyph top is (ascent - bitmap_top) pixels BELOW cursor.
    // "Below" means a negative topoffs value (Draw2DGraphic subtracts topoffs from t,
    // so negative topoffs increases t → moves image down toward the screen bottom).
    t->topoffs  = (short)(slot->bitmap_top - ascent);
    t->leftoffs = (short)(-slot->bitmap_left);

    g.mat = new Material(name.c_str(), t, SCALE, SCALE);
    g.mat->tex[0].min_filter = GL_LINEAR;
    g.mat->tex[0].mag_filter = GL_LINEAR;
    materials.RegisterMaterial(g.mat); // ensures ClearGLTextures() resets gl_id on context recreation
    return true;
}

float ttfont_t::DrawCharacter(float x, float y, int c, int flags)
{
    if (c < 32)
        return 0;
    else if (c < 256)
    {
        glyph_t &g = glyphcache[c];
        if (!g.mat && !BuildGlyph(g, c))
            return 0;
        g.mat->Draw(x, y, flags);
        return g.advance / SCALE;
    }
    else
    {
        glyph_t temp;
        if (!BuildGlyph(temp, c))
            return 0;
        temp.mat->Draw(x, y, flags);
        return temp.advance / SCALE;
    }
}

#endif

// Global pointer to TTF font for console use (separate from HUD font)
#ifndef NO_TTF
static ttfont_t *console_ttf = NULL;
#endif

// initialize the font system, load the fonts
void font_t::Init()
{
    int startlump, endlump;

    // "hud font"
    switch (game.mode)
    {
        case gm_heretic:
        case gm_hexen:
            startlump = fc.GetNumForName("FONTA01");
            endlump = fc.GetNumForName("FONTA59");
            break;
        default:
            startlump = fc.GetNumForName("STCFN033");
            endlump = fc.GetNumForName("STCFN095");
    }

    hud_font = new rasterfont_t(startlump, endlump);

    // "big font"
    if (fc.FindNumForName("FONTB_S") < 0)
        big_font = NULL;
    else
    {
        startlump = fc.FindNumForName("FONTB01");
        endlump = fc.GetNumForName("FONTB58");
        big_font = new rasterfont_t(startlump, endlump);
    }

#ifndef NO_TTF
    int lump = fc.FindNumForName("TTFCONSL");
    if (lump < 0)
    {
        CONS_Printf("FreeType: 'TTFCONSL' lump not found\n");
        return;
    }
    console_ttf = new ttfont_t(lump);
    CONS_Printf("FreeType: loaded %s %s\n",
                console_ttf->ft_face->family_name,
                console_ttf->ft_face->style_name);
#endif

}

// Get the TTF font for console rendering (or NULL if not available)
font_t *font_t::GetConsoleFont()
{
#ifdef NO_TTF
    return NULL;
#else
    return console_ttf;
#endif
}


