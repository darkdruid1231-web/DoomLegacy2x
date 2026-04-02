// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_data.cpp 570 2009-11-27 23:57:47Z smite-meister $
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
/// \brief Texture generation and caching. Colormap loading.

#include <math.h>
#include <png.h>

#define GL_GLEXT_PROTOTYPES 1
// Define GLchar before GL headers for MSYS2 MinGW compatibility
#ifndef GLchar
typedef char GLchar;
#endif
// On Windows, GLEW must be included first to provide extension function pointers
#if defined(_WIN32) || defined(__MINGW32__)
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#include "command.h"
#include "cvars.h"
#include "doomdata.h"
#include "doomdef.h"
#include "parser.h"

#include "g_actor.h"
#include "g_game.h"
#include "g_map.h"

#include "i_video.h"
#include "m_swap.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"

#include "w_wad.h"
#include "z_ascache.h"
#include "z_zone.h"

#include "hardware/oglshaders.h"

const byte *R_BuildGammaTable();
static int R_TransmapNumForName(const char *name);

// TODO after exiting a mapcluster, flush unnecessary graphics...
//  R_LoadTextures ();
//  R_FlushTextureCache();
//  R_ClearColormaps();

// faB: for debugging/info purpose
int flatmemory;
int spritememory;
int texturememory;

// faB: highcolor stuff
short color8to16[256]; // remap color index to highcolor rgb value
short *hicolormaps;    // test a 32k colormap remaps high -> high

//==================================================================
//   Texture cache
//==================================================================

/// Generates a Texture from a single data lump, deduces format.
Texture *texture_cache_t::Load(const char *name)
{
    int lump = fc.FindNumForName(name);
    if (lump < 0)
        return NULL;

    return LoadLump(name, lump);
}

/// Creates a Texture with a given name from a given lump.
Texture *texture_cache_t::LoadLump(const char *name, int lump)
{
    // Because Doom datatypes contain no magic numbers, we have to rely on heuristics to deduce the
    // format...
    Texture *t;

    byte data[8];
    fc.ReadLumpHeader(lump, &data, sizeof(data));
    int size = fc.LumpLength(lump);
    int name_len = strlen(name);

    // Good texture formats have magic numbers. First check if they exist.
    if (!png_sig_cmp(data, 0, sizeof(data)))
    {
        // it's PNG
        t = new PNGTexture(name, lump);
    }
    else if (data[0] == 0xff && data[1] == 0xd8 && data[2] == 0xff && data[3] == 0xe0)
    {
        // it's JPEG/JFIF
        t = new JPEGTexture(name, lump);
    }
    // For TGA, we use the filename extension.
    else if (name_len > 4 && !strcasecmp(&name[name_len - 4], ".tga"))
    {
        // it's TGA
        t = new TGATexture(name, lump);
    }
    // then try some common sizes for raw picture lumps
    else if (size == 320 * 200)
    {
        // Heretic/Hexen fullscreen picture
        t = new LumpTexture(name, lump, 320, 200);
    }
    else if (size == 4)
    {
        // God-damn-it! Heretic "F_SKY1" tries to be funny!
        t = new LumpTexture(name, lump, 2, 2);
    }
    else if (!strcasecmp(name, "AUTOPAGE"))
    {
        // how many different annoying formats can you invent, anyway?
        if (size % 320)
            I_Error("Size of AUTOPAGE (%d bytes) must be a multiple of 320!\n", size);
        t = new LumpTexture(name, lump, 320, size / 320);
    }
    else if (data[2] == 0 && data[6] == 0 && data[7] == 0)
    {
        // likely a pic_t (the magic number is inadequate)
        CONS_Printf("A pic_t image '%s' was found, but this format is no longer supported.\n",
                    name); // root 'em out!
        return NULL;
    }
    else
    {
        // finally assume a patch_t
        t = new PatchTexture(name, lump);
    }

    /*
      else
        {
          CONS_Printf(" Unknown texture format: lump '%8s' in the file '%s'.\n", name, fc.Name(lump
      >> 16)); return false;
        }
    */

    return t; // cache_t::Get() does the subsequent inserting into hash_map...
}

texture_cache_t textures;

//==================================================================
//  Utilities
//==================================================================

/// Store lists of lumps for F_START/F_END etc.
struct lumplist_t
{
    int wadfile;
    int firstlump;
    int numlumps;
};

static int R_CheckNumForNameList(const char *name, lumplist_t *ll, int listsize)
{
    for (int i = listsize - 1; i > -1; i--)
    {
        int lump = fc.FindNumForNameFile(name, ll[i].wadfile, ll[i].firstlump);
        if ((lump & 0xffff) >= (ll[i].firstlump + ll[i].numlumps) || lump == -1)
            continue;
        else
            return lump;
    }
    return -1;
}

// given an RGB triplet, returns the index of the nearest color in
// the current "zero"-palette using a quadratic distance measure.
// Thanks to quake2 source!
byte NearestColor(byte r, byte g, byte b)
{
    int bestdistortion = 256 * 256 * 4;
    int bestcolor = 0;

    for (int i = 0; i < 256; i++)
    {
        int dr = r - vid.palette[i].r;
        int dg = g - vid.palette[i].g;
        int db = b - vid.palette[i].b;
        int distortion = dr * dr + dg * dg + db * db;

        if (distortion < bestdistortion)
        {
            if (!distortion)
                return i;

            bestdistortion = distortion;
            bestcolor = i;
        }
    }

    return bestcolor;
}

// create best possible colormap from one palette to another
static byte *R_CreatePaletteConversionColormap(int wadnum)
{
    int i = fc.FindNumForNameFile("PLAYPAL", wadnum);
    if (i == -1)
    {
        // no palette available
        return NULL;
    }

    if (fc.LumpLength(i) < int(256 * sizeof(RGB_t)))
        I_Error("Bad PLAYPAL lump in file %d!\n", wadnum);

    byte *colormap = static_cast<byte *>(Z_Malloc(256, PU_STATIC, NULL));
    RGB_t *pal = static_cast<RGB_t *>(fc.CacheLumpNum(i, PU_STATIC));
    const byte *gamma_table = R_BuildGammaTable();

    for (i = 0; i < 256; i++)
        colormap[i] =
            NearestColor(gamma_table[pal[i].r], gamma_table[pal[i].g], gamma_table[pal[i].b]);

    Z_Free(pal);
    return colormap;
}

// applies a given colormap to a patch_t
static void R_ColormapPatch(patch_t *p, byte *colormap)
{
    for (int i = 0; i < p->width; i++)
    {
        post_t *post = reinterpret_cast<post_t *>(reinterpret_cast<byte *>(p) + p->columnofs[i]);

        while (post->topdelta != 0xff)
        {
            int count = post->length;
            for (int j = 0; j < count; j++)
                post->data[j] = colormap[post->data[j]];

            post = reinterpret_cast<post_t *>(&post->data[post->length + 1]);
        }
    }
}

//==================================================================
//  Texture
//==================================================================

Texture::Texture(const char *n, int l) : cacheitem_t(n)
{
    pixels = NULL;
    lump = l;
    leftoffs = topoffs = 0;
    width = height = 0;
    w_bits = h_bits = 0;

    gl_format = 0;
    gl_id = NOTEXTURE;
}

Texture::~Texture()
{
    ClearGLTexture();

    if (pixels)
        Z_Free(pixels);
}

// Spread opaque pixel colors into all transparent pixels (flood-fill style, multiple passes).
// This prevents GL_LINEAR fringing at sprite edges AND mipmap bleeding: without it, sampling
// between an opaque pixel and a transparent pixel gives the color of palette index 247
// (TRANSPARENTPIXEL), producing a dark/colored border artifact.
// After dilation all pixels have alpha=0 on fully-transparent regions, so they remain invisible,
// but GL_LINEAR and mipmap averages now read the correct edge color.
static void DilateEdgeColors(RGBA_t *rgba, int w, int h)
{
    int n = w * h;
    RGBA_t *src = static_cast<RGBA_t *>(Z_Malloc(sizeof(RGBA_t) * n, PU_STATIC, NULL));
    memcpy(src, rgba, sizeof(RGBA_t) * n);

    static const int dx[] = {-1, 1,  0, 0, -1, -1, 1,  1};
    static const int dy[] = { 0, 0, -1, 1, -1,  1, -1, 1};

    // Iterate until no more transparent pixels can be filled (flood-fill from opaque edges).
    // Worst case passes = max(w, h), but typically only a few for sprites.
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                if (src[y * w + x].alpha != 0)
                    continue; // already has color — nothing to do

                int r = 0, g = 0, b = 0, cnt = 0;
                for (int d = 0; d < 8; d++)
                {
                    int nx = x + dx[d], ny = y + dy[d];
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h)
                        continue;
                    const RGBA_t &nb = src[ny * w + nx];
                    if (nb.alpha == 0)
                        continue; // transparent neighbor — skip
                    r += nb.red;
                    g += nb.green;
                    b += nb.blue;
                    cnt++;
                }
                if (cnt > 0)
                {
                    // Mark as filled (alpha=1) in src so subsequent pixels in this pass
                    // can use it as a neighbor source.
                    src[y * w + x].red   = (byte)(r / cnt);
                    src[y * w + x].green = (byte)(g / cnt);
                    src[y * w + x].blue  = (byte)(b / cnt);
                    src[y * w + x].alpha = 1; // sentinel: filled this pass
                    rgba[y * w + x].red   = src[y * w + x].red;
                    rgba[y * w + x].green = src[y * w + x].green;
                    rgba[y * w + x].blue  = src[y * w + x].blue;
                    // rgba alpha stays 0 — pixel remains fully transparent
                    changed = true;
                }
            }
        }
    }

    Z_Free(src);
}

// This basic version of the virtual method is used for native indexed col-major formats.
void Texture::GLGetData()
{
    if (!pixels)
    {
        GetData(); // reads data into pixels as indexed, col-major
        byte *index_in = pixels;

        RGBA_t *result =
            static_cast<RGBA_t *>(Z_Malloc(sizeof(RGBA_t) * width * height, PU_TEXTURE, NULL));
        RGB_t *palette =
            static_cast<RGB_t *>(fc.CacheLumpNum(materials.GetPaletteLump(lump >> 16), PU_DAVE));

        for (int i = 0; i < width; i++)
            for (int j = 0; j < height; j++)
            {
                byte curbyte = *index_in++;
                RGB_t *rgb_in = &palette[curbyte];
                RGBA_t *rgba_out = &result[j * width + i]; // transposed
                rgba_out->red = rgb_in->r;
                rgba_out->green = rgb_in->g;
                rgba_out->blue = rgb_in->b;
                if (curbyte == TRANSPARENTPIXEL)
                    rgba_out->alpha = 0;
                else
                    rgba_out->alpha = 255;
            }

        Z_Free(palette);

        // Spread edge colors into transparent pixels so GL_LINEAR does not produce
        // a fringe of palette-index-247 color at the borders of masked sprites.
        DilateEdgeColors(result, width, height);

        // replace pixels by the RGBA row-major version
        Z_Free(pixels);
        pixels = reinterpret_cast<byte *>(result);
        gl_format = GL_RGBA;
    }
}

/// Creates a GL texture.
/// Uses the virtualized GLGetData().
GLuint Texture::GLPrepare()
{
    if (gl_id == NOTEXTURE)
    {
        GLGetData();

        glGenTextures(1, &gl_id);
        glBindTexture(GL_TEXTURE_2D, gl_id);
        // Sprites (masked textures) must clamp to edge: GL_REPEAT would wrap transparent
        // border pixels around to the opposite side, creating visible seam lines.
        // Wall/flat textures tile by UV coords so they keep GL_REPEAT.
        GLenum wrap = Masked() ? GL_CLAMP_TO_EDGE : GL_REPEAT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        gluBuild2DMipmaps(
            GL_TEXTURE_2D, GL_RGBA, width, height, gl_format, GL_UNSIGNED_BYTE, pixels);

        //  CONS_Printf("Created GL texture %d for %s.\n", gl_id, name);
        Z_Free(pixels); // no longer needed
        pixels = NULL;
    }

    return gl_id;
}

/// Returns true if texture existed and was deleted. False otherwise.
bool Texture::ClearGLTexture()
{
    if (gl_id != NOTEXTURE)
    {
        glDeleteTextures(1, &gl_id);
        gl_id = NOTEXTURE;
        return true;
    }
    return false;
}

// Basic form for GetColumn, uses virtualized GetData.
byte *Texture::GetColumn(fixed_t fcol)
{
    int col = fcol.floor() % width;

    if (col < 0)
        col += width; // wraparound

    return GetData() + col * height;
}

//==================================================================
//  LumpTexture
//==================================================================

// Flats etc.
LumpTexture::LumpTexture(const char *n, int l, int w, int h) : Texture(n, l)
{
    width = w;
    height = h;
    InitializeTexture();
}

/// This method handles the Doom flats and raw pages for OpenGL.
void LumpTexture::GLGetData()
{
    if (!pixels)
    {
        // load indexed data
        int len = fc.LumpLength(lump); // len must be width*height (or more...)
        byte *temp = static_cast<byte *>(Z_Malloc(len, PU_STATIC, NULL));
        fc.ReadLump(lump, temp); // to avoid unnecessary memcpy

        // allocate space for RGBA data, owner is pixels
        Z_Malloc(sizeof(RGBA_t) * width * height, PU_TEXTURE, (void **)(&pixels));

        // convert to RGBA
        byte *index_in = temp;
        RGB_t *palette =
            static_cast<RGB_t *>(fc.CacheLumpNum(materials.GetPaletteLump(lump >> 16), PU_DAVE));
        RGBA_t *rgba_out = reinterpret_cast<RGBA_t *>(pixels);

        for (int i = 0; i < width; i++)
            for (int j = 0; j < height; j++)
            {
                byte curbyte = *index_in++;
                RGB_t *rgb_in = &palette[curbyte]; // untransposed
                rgba_out->red = rgb_in->r;
                rgba_out->green = rgb_in->g;
                rgba_out->blue = rgb_in->b;
                if (curbyte == TRANSPARENTPIXEL)
                    rgba_out->alpha = 0;
                else
                    rgba_out->alpha = 255;

                rgba_out++;
            }

        Z_Free(palette);

        // Spread edge colors into transparent pixels to prevent GL_LINEAR fringing.
        DilateEdgeColors(reinterpret_cast<RGBA_t *>(pixels), width, height);

        Z_Free(temp); // free indexed data
        gl_format = GL_RGBA;
    }
}

/// This method handles the Doom flats and raw pages.
byte *LumpTexture::GetData()
{
    if (!pixels)
    {
        int len = fc.LumpLength(lump);
        Z_Malloc(len, PU_TEXTURE, (void **)(&pixels));

        byte *temp = static_cast<byte *>(fc.CacheLumpNum(lump, PU_STATIC));

        // transposed to col-major order
        int dest = 0;
        for (int i = 0; i < len; i++)
        {
            pixels[dest] = temp[i];
            dest += height;
            if (dest >= len)
                dest -= len - 1; // next column
        }
        Z_Free(temp);

        // do a palette conversion if needed
        byte *colormap = materials.GetPaletteConv(lump >> 16);
        if (colormap)
            for (int i = 0; i < len; i++)
                pixels[i] = colormap[pixels[i]];

        // convert to high color
        // short pix16 = ((color8to16[*data++] & 0x7bde) + ((i<<9|j<<4) & 0x7bde))>>1;
    }

    return pixels;
}

//==================================================================
//  PatchTexture
//==================================================================

// Clip and draw a column from a patch into a cached post.
static void R_DrawColumnInCache(column_t *col, byte *cache, int originy, int cacheheight)
{
    while (col->topdelta != 0xff)
    {
        byte *source = col->data; // go to the data
        int count = col->length;
        int position = originy + col->topdelta;

        if (position < 0)
        {
            count += position;
            source -= position;
            position = 0;
        }

        if (position + count > cacheheight)
            count = cacheheight - position;

        if (count > 0)
            memcpy(cache + position, source, count);

        col = reinterpret_cast<column_t *>(&col->data[col->length + 1]);
    }
}

PatchTexture::PatchTexture(const char *n, int l) : Texture(n, l)
{
    patch_t p;
    fc.ReadLumpHeader(lump, &p, sizeof(patch_t));
    width = SHORT(p.width);
    height = SHORT(p.height);
    leftoffs = SHORT(p.leftoffset);
    topoffs = SHORT(p.topoffset);

    InitializeTexture();
    // nothing more is needed until the texture is Generated.

    patch_data = NULL;

    if (fc.LumpLength(lump) <= width * 4 + 8)
        I_Error("PatchTexture: lump %d (%s) is invalid\n", l, n);
}

PatchTexture::~PatchTexture()
{
    if (patch_data)
        Z_Free(patch_data);
}

/// sets patch_data
patch_t *PatchTexture::GeneratePatch()
{
    if (!patch_data)
    {
        int len = fc.LumpLength(lump);
        patch_t *p = static_cast<patch_t *>(Z_Malloc(len, PU_TEXTURE, (void **)&patch_data));

        // to avoid unnecessary memcpy
        fc.ReadLump(lump, patch_data);

        // [segabor] necessary endianness conversion for p
        // [smite] should not be necessary, because the other fields are never used

        for (int i = 0; i < width; i++)
            p->columnofs[i] = LONG(p->columnofs[i]);

        // do a palette conversion if needed
        byte *colormap = materials.GetPaletteConv(lump >> 16);
        if (colormap)
        {
            p->width = SHORT(p->width);
            R_ColormapPatch(p, colormap);
        }
    }

    return reinterpret_cast<patch_t *>(patch_data);
}

/// sets pixels (which implies that patch_data is also set)
byte *PatchTexture::GetData()
{
    if (!pixels)
    {
        // we need to draw the patch into a rectangular bitmap in column-major order
        int len = width * height;
        Z_Malloc(len, PU_TEXTURE, (void **)&pixels);
        memset(pixels, TRANSPARENTPIXEL, len);

        patch_t *p = GeneratePatch();

        for (int col = 0; col < width; col++)
        {
            column_t *patchcol = reinterpret_cast<column_t *>(patch_data + p->columnofs[col]);
            R_DrawColumnInCache(patchcol, pixels + col * height, 0, height);
        }
    }

    return pixels;
}

column_t *PatchTexture::GetMaskedColumn(fixed_t fcol)
{
    int col = fcol.floor() % width;

    if (col < 0)
        col += width; // wraparound

    patch_t *p = GeneratePatch();
    return reinterpret_cast<column_t *>(patch_data + p->columnofs[col]);
}

//==================================================================
//  DoomTexture
//==================================================================

DoomTexture::DoomTexture(const char *n, int l, const maptexture_t *mtex) : Texture(n, l)
{
    patchcount = SHORT(mtex->patchcount);
    patches = static_cast<texpatch_t *>(Z_Malloc(sizeof(texpatch_t) * patchcount, PU_TEXTURE, 0));

    width = SHORT(mtex->width);
    height = SHORT(mtex->height);

    InitializeTexture();
    widthmask = (1 << w_bits) - 1;

    patch_data = NULL;
}

DoomTexture::~DoomTexture()
{
    Z_Free(patches);

    if (patch_data)
        Z_Free(patch_data);
}

// TODO better DoomTexture handling?
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.

//   Allocate space for full size texture, either single patch or 'composite'
//   Build the full textures from patches.
//   The texture caching system is a little more hungry of memory, but has
//   been simplified for the sake of highcolor, dynamic lighting, & speed.

patch_t *DoomTexture::GeneratePatch()
{
    // patchcount must be 1! no splicing!
    // single-patch textures can have holes in it and may be used on
    // 2-sided lines so they need to be kept in patch_t format
    if (!patch_data)
    {
        CONS_Printf("Generating patch for '%s'\n", name);
        texpatch_t *tp = patches;
        int blocksize = fc.LumpLength(tp->patchlump);
        // CONS_Printf ("R_GenTex SINGLE %.8s size: %d\n",name,blocksize);

        patch_t *p = static_cast<patch_t *>(Z_Malloc(blocksize, PU_TEXTURE, (void **)&patch_data));
        texturememory += blocksize;
        fc.ReadLump(tp->patchlump, patch_data);

        p->width = SHORT(p->width); // endianness...

        // FIXME should use patch width here? texture may be wider!
        if (width > p->width)
        {
            CONS_Printf("masked tex '%s' too wide\n", name);
            width = p->width;
            InitializeTexture();
            widthmask = (1 << w_bits) - 1;
        }

        // use the patch's column lookup (columnofs is reserved for the raw bitmap version!)
        // do not skip post_t info by default
        for (int i = 0; i < width; i++)
            p->columnofs[i] = LONG(p->columnofs[i]);

        // do a palette conversion if needed
        byte *colormap = materials.GetPaletteConv(tp->patchlump >> 16);
        if (colormap)
            R_ColormapPatch(p, colormap);
    }

    return reinterpret_cast<patch_t *>(patch_data);
}

byte *DoomTexture::GetData()
{
    if (!pixels)
    {
        // CONS_Printf("Generating data for '%s'\n", name);
        int i;
        // multi-patch (or 'composite') textures are stored as a simple bitmap

        int size = width * height;
        int blocksize =
            size + width * sizeof(Uint32); // first raw col-major data, then columnofs table
        // CONS_Printf ("R_GenTex MULTI  %.8s size: %d\n",name,blocksize);

        Z_Malloc(blocksize, PU_TEXTURE, (void **)&pixels);
        texturememory += blocksize;

        // generate column offset lookup table
        columnofs = reinterpret_cast<Uint32 *>(pixels + size);
        for (i = 0; i < width; i++)
            columnofs[i] = i * height;

        // prepare texture bitmap
        memset(pixels, TRANSPARENTPIXEL, size);

        texpatch_t *tp;
        // Composite the patches together.
        for (i = 0, tp = patches; i < patchcount; i++, tp++)
        {
            patch_t *p = static_cast<patch_t *>(fc.CacheLumpNum(tp->patchlump, PU_STATIC));
            int x1 = tp->originx;
            int x2 = x1 + SHORT(p->width);

            int x = x1;
            if (x < 0)
                x = 0;

            if (x2 > width)
                x2 = width;

            for (; x < x2; x++)
            {
                column_t *patchcol = reinterpret_cast<column_t *>(reinterpret_cast<byte *>(p) +
                                                                  LONG(p->columnofs[x - x1]));
                R_DrawColumnInCache(patchcol, pixels + columnofs[x], tp->originy, height);
            }

            Z_Free(p);
        }

        // TODO do a palette conversion if needed
    }

    return pixels;
}

// returns a pointer to column-major raw data
byte *DoomTexture::GetColumn(fixed_t fcol)
{
    int col = fcol.floor();

    return GetData() + columnofs[col & widthmask];
}

column_t *DoomTexture::GetMaskedColumn(fixed_t fcol)
{
    if (patchcount == 1)
    {
        int col = fcol.floor();
        patch_t *p = GeneratePatch();
        return reinterpret_cast<column_t *>(patch_data + p->columnofs[col & widthmask]);
    }
    else
        return NULL;
}

//==================================================================
//  Materials
//==================================================================

Material::Material(const char *name) : cacheitem_t(name)
{
    id_number = -1;
    shader = NULL;
    glossiness = 1.0f;
    specularlevel = 1.0f;
    mode = TEX_lod;
    // The rest are taken care of by InitializeMaterial() later when Textures have been attached.
}

// Single-texture Materials. tex.size() guaranteed to be 1 after this constructor.
Material::Material(const char *name, Texture *t, float xs, float ys, material_class_t m) : cacheitem_t(name)
{
    id_number = -1;
    shader = NULL;
    glossiness = 1.0f;
    specularlevel = 1.0f;
    mode = m;

    tex.resize(1);
    tex[0].t = t;
    tex[0].xscale = xs;
    tex[0].yscale = ys;

    // t is used by this material, so
    t->AddRef(); // NOTE: we don't currently use links for Textures, this functionality is subsumed
                 // in material_cache_t::Get

    InitializeMaterial();
}

Material::~Material()
{
    int n = tex.size(); // number of texture units
    for (int i = 0; i < n; i++)
        if (tex[i].t) tex[i].t->Release();
}

Material::TextureRef::TextureRef()
{
    // default scaling and OpenGL params
    t = NULL;
    xscale = yscale = 1;

    mag_filter = 0; // If these are zero, take values from consvars.
    min_filter = 0;
    max_anisotropy = 0.0;
}

void Material::TextureRef::GLSetTextureParams(material_class_t mode)
{
    glBindTexture(GL_TEXTURE_2D, t->GLPrepare()); // bind the texture

    GLint magf = mag_filter ? mag_filter : (cv_grfiltermode.value ? GL_LINEAR : GL_NEAREST);
    // HACK, relies on the order of the GL numeric constants which should be always the same
    GLint minf = min_filter ? min_filter : GL_NEAREST_MIPMAP_NEAREST + cv_grfiltermode.value;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minf);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    max_anisotropy ? max_anisotropy : cv_granisotropy.value);

    // Use GL_CLAMP_TO_EDGE for sprites and LOD (menu/HUD graphics).
    // Use GL_REPEAT for wall and floor textures (tiling).
    GLenum wrap = (mode == TEX_wall || mode == TEX_floor) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
}

int Material::GLUse()
{
    // Bind textures BEFORE activating the shader.
    // gluBuild2DMipmaps (called on first GLPrepare) resets GL_CURRENT_PROGRAM to 0,
    // so we must activate the shader after all texture uploads are done.
    int n = tex.size(); // number of texture units
    for (int i = 0; i < n; i++)
    {
        if (!tex[i].t)
        {
            if (shader && i >= 1) BindPBRNeutral(i); // neutral for empty slot (1=flat normal, 2-7=PBR)
            continue;
        }
        glActiveTexture(GL_TEXTURE0 + i); // activate correct texture unit
        tex[i].GLSetTextureParams(mode);  // pass material mode for wrap mode
    }
    // Bind neutrals for any PBR slots beyond what this material defines
    if (shader)
        for (int i = n; i <= 7; i++)
            BindPBRNeutral(i);

    if (n > 1)
        glActiveTexture(GL_TEXTURE0); // restore default unit

    if (shader)
    {
        shader->Use();
        shader->SetUniforms();
        shader->SetPerMaterialUniforms(glossiness, specularlevel);
    }
    else
        ShaderProg::DisableShaders();

    return n;
}

//==================================================================
//  Material cache
//==================================================================

material_cache_t materials;

material_cache_t::material_cache_t()
{
    default_item = NULL;
}

material_cache_t::~material_cache_t()
{
    if (default_item)
        delete default_item;
}

void material_cache_t::SetDefaultItem(const char *name)
{
    if (default_item)
        CONS_Printf("Material_Cache: Replacing default_item!\n");
    // TODO delete the old default item?

    textures.SetDefaultItem(name);
    Texture *t = textures.default_item;
    if (!t)
        I_Error("Material_Cache: New default_item '%s' not found!\n", name);

    default_item = new Material(name, t);
}

void material_cache_t::Clear()
{
    new_tex.Clear();
    doom_tex.Clear();
    flat_tex.Clear();
    sprite_tex.Clear();
    lod_tex.Clear();

    all_materials.clear();
}

void material_cache_t::ClearGLTextures()
{
    int count = 0;
    for (material_iterator_t ti = all_materials.begin(); ti != all_materials.end(); ti++)
    {
        Material *m = *ti;
        if (m)
        {
            int n = m->tex.size();
            for (int k = 0; k < n; k++)
                if (m->tex[k].t && m->tex[k].t->ClearGLTexture())
                    count++;
        }
    }

    if (count)
        CONS_Printf("Cleared %d OpenGL textures.\n", count);
}

/// Creates a new blank Material in cache or returns an existing one for updating. Does not change
/// reference counts. For NTEXTURE.
Material *material_cache_t::Edit(const char *name, material_class_t mode, bool create)
{
    Material *t;

    switch (mode)
    {
        case TEX_wall: // walls
            if (!(t = new_tex.Find(name)))
                if (!(t = doom_tex.Find(name)))
                    t = flat_tex.Find(name);
            break;

        case TEX_floor: // floors, ceilings
            if (!(t = new_tex.Find(name)))
                if (!(t = flat_tex.Find(name)))
                    t = doom_tex.Find(name);
            break;

        case TEX_sprite: // sprite frames
            t = sprite_tex.Find(name);
            break;

        case TEX_lod:
            I_Error("material_cache_t::Edit() cannot be used with TEX_lod!\n");
            return NULL;
    }

    if (!t && create)
    {
        Material *m = new Material(name);
        if (mode == TEX_sprite)
            sprite_tex.Insert(m);
        else
            new_tex.Insert(m);

        Register(m);
        return m;
    }

    return t;
}

/// Build a single-Texture Material during startup, insert it into a given source.
/// Texture is assumed not to be in cache before call (unless there is a namespace overlap).
Material *material_cache_t::BuildMaterial(Texture *t, cachesource_t<Material> &source, material_class_t mode, bool h_start)
{
    if (!t)
        return NULL;

    string name = t->GetName(); // char* won't do since we may change the Texture's name

    if (h_start) // H_START / hires replacement: must find an existing original first
    {
        // Check BEFORE inserting — if this source has no original, leave the
        // texture untouched so subsequent calls with other sources can try.
        Material *m = source.Find(name.c_str());
        if (!m)
            return NULL;

        // Original found — now commit the texture into the cache and replace.
        if (textures.Exists(name.c_str()))
            t->SetName((name + "_xxx").c_str());
        if (!textures.Insert(t))
            CONS_Printf("Error: Overlapping Texture names '%s'!\n", t->GetName());

        // Take over the original, scale so that worldsize stays the same.
        Material::TextureRef &r = m->tex[0];
        r.t->Release();
        t->AddRef();
        r.t = t;
        r.xscale = t->width / r.worldwidth;
        r.yscale = t->height / r.worldheight;
        return m;
    }
    // Normal (non-hires) case: insert texture, then create material if none exists yet.
    // All loading loops iterate nwads-1 down to 0 so later-loaded (PWAD) files are
    // processed first and have priority.  When an earlier-loaded (IWAD) file provides a
    // duplicate name, we keep the already-inserted PWAD version.
    if (textures.Exists(name.c_str()))
    {
        CONS_Printf("Overlap in Texture names '%s'!\n", name.c_str());
        t->SetName((name + "_xxx").c_str());
    }

    if (!textures.Insert(t))
        CONS_Printf("Error: Overlapping Texture names '%s'!\n", t->GetName());

    // see if we already have a Material with this name
    Material *m = source.Find(name.c_str());

    if (!m)
    {
        m = new Material(name.c_str(), t, 1.0f, 1.0f, mode); // create a new Material
        source.Insert(m);
        Register(m);
    }
    // else: material already exists from a higher-priority (later-loaded) file — keep it.

    return m;
}

void material_cache_t::InitPaletteConversion()
{
    // create the palette conversion colormaps
    unsigned i;
    for (i = 0; i < palette_conversion.size(); i++)
        if (palette_conversion[i])
            Z_Free(palette_conversion[i]);

    unsigned n = fc.Size(); // number of resource files
    palette_conversion.resize(n);
    palette_lump.resize(n);

    // For OpenGL, just find the correct palettes to use for each file
    int def_palette = fc.FindNumForName("PLAYPAL");
    for (i = 0; i < n; i++)
    {
        palette_conversion[i] = R_CreatePaletteConversionColormap(i);
        int lump = fc.FindNumForNameFile("PLAYPAL", i);
        palette_lump[i] = (lump < 0) ? def_palette : lump;
    }
}

/// Returns an existing Material, or tries creating it if nonexistant.
Material *material_cache_t::Get8char(const char *name, material_class_t mode)
{
    char name8[9]; // NOTE texture names in Doom map format are max 8 chars long and not always
                   // NUL-terminated!

    strncpy(name8, name, 8);
    name8[8] = '\0';
    strupr(name8); // TODO unfortunate, could be avoided if we used a non-case-sensitive string
                   // comparison functor...

    return Get(name8, mode);
};

/// Returns pointer to an existing Material, or tries creating it if nonexistant.
Material *material_cache_t::Get(const char *name, material_class_t mode)
{
    // "No texture" marker.
    if (!name || name[0] == '-')
        return NULL;

    Material *t;

    switch (mode)
    {
        case TEX_wall: // walls
            if (!(t = new_tex.Find(name)))
                if (!(t = doom_tex.Find(name)))
                    t = flat_tex.Find(name);
            break;

        case TEX_floor: // floors, ceilings
            if (!(t = new_tex.Find(name)))
                if (!(t = flat_tex.Find(name)))
                    t = doom_tex.Find(name);
            break;

        case TEX_sprite: // sprite frames
            t = sprite_tex.Find(name);
            break;

        case TEX_lod: // menu, HUD, console, background images
            if (!(t = new_tex.Find(name)))
                if (!(t = lod_tex.Find(name)))
                    if (!(t = doom_tex.Find(name)))
                    {
                        // Not found there either, try loading on demand
                        Texture *tex = textures.Load(name);
                        if (tex)
                            t = BuildMaterial(tex, lod_tex, TEX_lod); // wasteful...
                    }
            break;
    }

    if (!t)
    {
        // Item not found at all.
        // Some nonexistant items are asked again and again.
        // We use a special cacheitem_t to link their names to the default item.
        t = new Material(name);
        t->MakeLink();
        // This is a dummy Material which has no textures attached, so the tex vector remains empty.
        // Direct pointers to dummy Materials must not be passed outside material_cache_t.

        // NOTE insertion to cachesource only, not to id-map
        if (mode == TEX_sprite)
            sprite_tex.Insert(t);
        else
            new_tex.Insert(t);
    }

    if (t->AddRef())
    {
        // a "link" to default_item
        if (default_item)
        {
            default_item->AddRef();
            return default_item;
        }
        return NULL;
    }
    else
        return t;
}

/// Shorthand.
Material *material_cache_t::GetLumpnum(int n)
{
    return Get(fc.FindNameForNum(n), TEX_lod);
}

// semi-hack for linedeftype 242 and the like, where the
// "texture name" field can hold a colormap name instead.
Material *material_cache_t::GetMaterialOrColormap(const char *name, fadetable_t *&cmap)
{
    // "NoTexture" marker. No texture, no colormap.
    if (name[0] == '-')
    {
        cmap = NULL;
        return NULL;
    }

    fadetable_t *temp = R_FindColormap(name); // check C_*... lists

    if (temp)
    {
        // it is a colormap lumpname, not a texture
        cmap = temp;
        return NULL;
    }

    cmap = NULL;

    // it could be a texture, let's check
    return Get8char(name, TEX_wall);
}

// semi-hack for linedeftype 260, where the
// "texture name" field can hold a transmap name instead.
Material *material_cache_t::GetMaterialOrTransmap(const char *name, int &map_num)
{
    // "NoTexture" marker. No texture, no colormap.
    if (name[0] == '-')
    {
        map_num = -1;
        return NULL;
    }

    int temp = R_TransmapNumForName(name);

    if (temp >= 0)
    {
        // it is a transmap lumpname, not a texture
        map_num = temp;
        return NULL;
    }

    map_num = -1;

    // it could be a texture, let's check
    return Get8char(name, TEX_wall);
}

bool Read_NTEXTURE(int lump);

/// ZDoom-compatible PK3 namespace directories.
enum pk3_ns_t { PK3_NONE, PK3_TEXTURES, PK3_PATCHES, PK3_FLATS, PK3_SPRITES, PK3_HIRES, PK3_HIRES_SPRITES, PK3_GRAPHICS };

/// Returns the GZDoom-compatible filter name for the current IWAD.
/// This is the dotted path that filter/ directory entries are matched against.
static const char *GetIWADFilterName()
{
    switch (game.mode)
    {
        case gm_doom1:
            // Registered Doom (3 episodes, no E4M1) vs Ultimate Doom (4 episodes).
            return (fc.FindNumForName("E4M1") != -1) ? "doom.id.doom1" : "doom.id.doom1.registered";
        case gm_doom2:
            return "doom.id.doom2";  // TODO: detect plutonia/tnt by unique lumps
        case gm_heretic:
            return "heretic";
        case gm_hexen:
            return "hexen";
        default:
            return "";
    }
}

/// GZDoom filter matching rule: a filter folder matches the current IWAD if the folder name
/// is a prefix of the IWAD's full filter string (dot-separated components).
/// e.g. "doom.id.doom1" is a prefix of "doom.id.doom1.registered" → matches registered doom.
/// e.g. "doom.id.doom1.registered" is NOT a prefix of "doom.id.doom1" → no match for ultimate doom.
static bool PK3_FilterMatchesGame(const char *gname, size_t glen)
{
    const char *iwad = GetIWADFilterName();
    size_t iwad_len = strlen(iwad);
    if (!iwad_len || glen > iwad_len) return false;
    if (strncasecmp(gname, iwad, glen) != 0) return false;
    // The filter folder must end at a component boundary.
    return (glen == iwad_len || iwad[glen] == '.');
}

/// Count the number of dot-separated components in a filter name (e.g. "doom.id.doom1" = 3).
static int PK3_FilterDepth(const char *gname, size_t glen)
{
    int depth = 1;
    for (size_t k = 0; k < glen; k++)
        if (gname[k] == '.') depth++;
    return depth;
}

/// Given a full ZIP entry path (e.g. "textures/walls/WALL01.png"), identify its
/// namespace and fill stem with the uppercased basename without extension.
/// Returns PK3_NONE if the entry is not in a recognized namespace.
static pk3_ns_t PK3_GetNamespace(const char *fullpath, char *stem, size_t stem_size)
{
    static const struct { const char *prefix; size_t len; pk3_ns_t ns; } ns_table[] = {
        { "textures/",       9,  PK3_TEXTURES },
        { "patches/",        8,  PK3_PATCHES  },
        { "flats/",          6,  PK3_FLATS    },
        { "sprites/",        7,  PK3_SPRITES  },
        // graphics/ is GZDoom's namespace for fullscreen/HUD images (TITLEPIC, HELP1, WIMAPs, etc.)
        // lumps/ is GZDoom's generic lump-replacement namespace (same treatment here).
        { "graphics/",       9,  PK3_GRAPHICS },
        { "lumps/",          6,  PK3_GRAPHICS },
        // hires/sprites/ scales HD sprites to match original world dimensions.
        // All other hires/ sub-dirs fall through to the generic hires/ entry below.
        { "hires/sprites/",  14, PK3_HIRES_SPRITES },
        // Generic hires/ (including hires/textures/, hires/flats/, etc.):
        // replace existing materials with dimension scaling (h_start=true).
        { "hires/",          6,  PK3_HIRES         },
    };

    // Strip optional "filter/GAME/" prefix (ZDoom filter/ convention for IWAD-specific resources).
    // e.g. "filter/doom/hires/textures/STARTAN3.png" -> "hires/textures/STARTAN3.png"
    const char *check = fullpath;
    if (strncasecmp(check, "filter/", 7) == 0)
    {
        check += 7; // skip "filter/"
        const char *slash = strchr(check, '/');
        if (!slash) return PK3_NONE; // bare "filter/GAME" with no further path
        check = slash + 1; // skip "GAME/"
    }

    for (size_t i = 0; i < sizeof(ns_table)/sizeof(ns_table[0]); i++)
    {
        if (strncasecmp(check, ns_table[i].prefix, ns_table[i].len) != 0)
            continue;

        const char *base = check + ns_table[i].len;
        // Support subdirectories: use only the final component
        const char *slash = strrchr(base, '/');
        if (slash) base = slash + 1;

        // Strip extension
        const char *dot = strrchr(base, '.');
        size_t copy_len = dot ? (size_t)(dot - base) : strlen(base);
        if (copy_len == 0 || copy_len >= stem_size)
            return PK3_NONE;

        strncpy(stem, base, copy_len);
        stem[copy_len] = '\0';
        strupr(stem);
        return ns_table[i].ns;
    }
    return PK3_NONE;
}

/// Initializes the material cache, fills the cachesource_t containers with Material objects.
/*!
  Follows the JDS texture standard.
  The reason we use texture_cache_t::LoadLump instead of texture_cache_t::Get here is to
  avoid namespace overlaps by making sure a new Texture is created each time from a particular lump.
  material_cache_t::BuildMaterial takes care of inserting the newly created Texture into the texture
  cache, changing the name if it is already taken.
*/
int material_cache_t::ReadTextures()
{
    int i, lump;
    int num_textures = 0;

    char name8[9];
    name8[8] = 0; // NUL-terminated

    // TEXTUREx/PNAMES:
    {
        // Load the patch names from the PNAMES lump
        struct pnames_t
        {
            Uint32 count;
            char names[][8]; // list of 8-byte patch names
        } __attribute__((packed));

        lump = fc.GetNumForName("PNAMES");
        pnames_t *pnames = static_cast<pnames_t *>(fc.CacheLumpNum(lump, PU_STATIC));
        int numpatches = LONG(pnames->count);

        if (devparm)
            CONS_Printf(" PNAMES: lump %d:%d, %d patches\n", lump >> 16, lump & 0xffff, numpatches);

        if (fc.LumpLength(lump) != 4 + 8 * numpatches)
            I_Error("Corrupted PNAMES lump.\n");

        vector<int> patchlookup(numpatches); // mapping from patchnumber to lumpnumber

        for (i = 0; i < numpatches; i++)
        {
            strncpy(name8, pnames->names[i], 8);
            patchlookup[i] = fc.FindNumForName(name8);
            if (patchlookup[i] < 0)
                CONS_Printf(" Patch '%s' (%d) not found!\n", name8, i);
        }

        // Load the map texture definitions from textures.lmp.
        // The data is contained in one or two lumps,
        //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
        int *maptex, *maptex1, *maptex2;

        lump = fc.GetNumForName("TEXTURE1");
        maptex = maptex1 = static_cast<int *>(fc.CacheLumpNum(lump, PU_STATIC));
        int numtextures1 = LONG(*maptex);
        int maxoff = fc.LumpLength(lump);
        if (devparm)
            CONS_Printf(
                " TEXTURE1: lump %d:%d, %d textures\n", lump >> 16, lump & 0xffff, numtextures1);
        int *directory = maptex + 1;

        int numtextures2, maxoff2;
        if (fc.FindNumForName("TEXTURE2") != -1)
        {
            lump = fc.GetNumForName("TEXTURE2");
            maptex2 = static_cast<int *>(fc.CacheLumpNum(lump, PU_STATIC));
            numtextures2 = LONG(*maptex2);
            maxoff2 = fc.LumpLength(lump);
            if (devparm)
                CONS_Printf(" TEXTURE2: lump %d:%d, %d textures\n",
                            lump >> 16,
                            lump & 0xffff,
                            numtextures2);
        }
        else
        {
            maptex2 = NULL;
            numtextures2 = 0;
            maxoff2 = 0;
        }

        int numtextures = numtextures1 + numtextures2;

        for (i = 0; i < numtextures; i++, directory++)
        {
            // only during game startup
            // if (!(i&63))
            //     CONS_Printf (".");

            if (i == numtextures1)
            {
                // Start looking in second texture file.
                maptex = maptex2;
                maxoff = maxoff2;
                directory = maptex + 1;
            }

            // offset to the current texture in TEXTURESn lump
            int offset = LONG(*directory);

            if (offset > maxoff)
                I_Error("ReadTextures: bad texture directory");

            // maptexture describes texture name, size, and
            // used patches in z order from bottom to top
            maptexture_t *mtex =
                reinterpret_cast<maptexture_t *>(reinterpret_cast<byte *>(maptex) + offset);
            strncpy(name8, mtex->name, 8);
            DoomTexture *tex =
                new DoomTexture(name8, lump, mtex); // lump is not so crucial here, it merely needs
                                                    // to belong to the correct datafile

            mappatch_t *mp = mtex->patches;
            DoomTexture::texpatch_t *p = tex->patches;

            for (int j = 0; j < tex->patchcount; j++, mp++, p++)
            {
                p->originx = SHORT(mp->originx);
                p->originy = SHORT(mp->originy);
                p->patchlump = patchlookup[SHORT(mp->patch)];
                if (p->patchlump == -1)
                    I_Error("ReadTextures: Missing patch %d in texture %.8s (%d)\n",
                            SHORT(mp->patch),
                            mtex->name,
                            i);
            }

            Material *m = BuildMaterial(tex, doom_tex, TEX_wall);

            // ZDoom extension, scaling. FIXME this could be done more gracefully (params to
            // BuildMaterial?)
            m->tex[0].xscale = mtex->xscale ? mtex->xscale / 8.0 : 1;
            m->tex[0].yscale = mtex->yscale ? mtex->yscale / 8.0 : 1;
            m->InitializeMaterial(); // again...
        }

        Z_Free(maptex1);
        if (maptex2)
            Z_Free(maptex2);

        // then rest of PNAMES
        for (i = 0; i < numpatches; i++)
            if (patchlookup[i] >= 0)
            {
                strncpy(name8, pnames->names[i], 8);
                if (doom_tex.Find(name8))
                    continue; // already defined in TEXTUREx

                PatchTexture *tex = new PatchTexture(name8, patchlookup[i]);
                BuildMaterial(tex, doom_tex, TEX_wall);
                numtextures++;

                // CONS_Printf(" Bare PNAMES texture '%s' found!\n", name8);
            }

        Z_Free(pnames);

        num_textures += numtextures;
    }

    int nwads = fc.Size();

    // TX_START
    // later files override earlier ones
    for (i = nwads - 1; i >= 0; i--)
    {
        int lump = fc.FindNumForNameFile("TX_START", i, 0);
        if (lump == -1)
            continue;
        else
            lump++;

        int end = fc.FindNumForNameFile("TX_END", i, 0);
        if (end == -1)
        {
            CONS_Printf(" TX_END missing in file '%s'.\n", fc.Name(i));
            continue; // no TX_END, nothing accepted
        }

        for (; lump < end; lump++)
            if (BuildMaterial(textures.LoadLump(fc.FindNameForNum(lump), lump), new_tex, TEX_lod))
                num_textures++;
    }

    // F_START, FF_START
    for (i = nwads - 1; i >= 0; i--)
    {
        // Only accept flats between F(F)_START and F(F)_END

        int lump = fc.FindNumForNameFile("F_START", i, 0);
        if (lump == -1)
            lump = fc.FindNumForNameFile("FF_START", i, 0); // deutex compatib.

        if (lump == -1)
            continue; // no flats in this wad
        else
            lump++; // just after F_START

        int end = fc.FindNumForNameFile("F_END", i, 0);
        if (end == -1)
            end = fc.FindNumForNameFile("FF_END", i, 0); // deutex compatib.

        if (end == -1)
        {
            CONS_Printf(" F_END missing in file '%s'.\n", fc.Name(i));
            continue; // no F_END, no flats accepted
        }

        for (; lump < end; lump++)
        {
            LumpTexture *t;

            const char *name = fc.FindNameForNum(lump); // now always NUL-terminated
            if (flat_tex.Find(name))
                continue; // already defined

            int size = fc.LumpLength(lump);

            if (size == 64 * 64 || // normal flats
                size == 65 * 64)   // Damn you, Heretic animated flats!
                // Flat is 64*64 bytes of raw paletted picture data in one lump
                t = new LumpTexture(name, lump, 64, 64);
            else if (size == 128 * 64) // Some Hexen flats (X_001-X_011) are larger! Why?
                t = new LumpTexture(name, lump, 128, 64);
            else if (size == 128 * 128)
                t = new LumpTexture(name, lump, 128, 128);
            else if (size == 256 * 128)
                t = new LumpTexture(name, lump, 256, 128);
            else if (size == 256 * 256)
                t = new LumpTexture(name, lump, 256, 256);
            else
            {
                if (size != 0) // markers are OK
                    CONS_Printf(
                        " Unknown flat type: lump '%8s' in the file '%s'.\n", name, fc.Name(i));
                continue;
            }

            BuildMaterial(t, flat_tex, TEX_floor);
            num_textures++;
        }
    }

    // S_START, SS_START
    for (i = nwads - 1; i >= 0; i--)
    {
        // Only accept patch_t's between S(S)_START and S(S)_END

        int lump = fc.FindNumForNameFile("S_START", i, 0);
        if (lump == -1)
            lump = fc.FindNumForNameFile("SS_START", i, 0); // deutex compatib.

        if (lump == -1)
            continue; // no spriteframes in this wad
        else
            lump++; // just after S_START

        int end = fc.FindNumForNameFile("S_END", i, 0);
        if (end == -1)
            end = fc.FindNumForNameFile("SS_END", i, 0); // deutex compatib.

        if (end == -1)
        {
            CONS_Printf(" S_END missing in file '%s'.\n", fc.Name(i));
            continue; // no S_END, no spriteframes accepted
        }

        for (; lump < end; lump++)
            if (BuildMaterial(textures.LoadLump(fc.FindNameForNum(lump), lump), sprite_tex, TEX_sprite))
                num_textures++;
    }

    // H_START: Variable-resolution textures that are scaled so they match
    // the size of a corresponding texture in TEXTUREx or F_START.
    for (i = nwads - 1; i >= 0; i--)
    {
        int lump = fc.FindNumForNameFile("H_START", i, 0);
        if (lump == -1)
            continue;
        else
            lump++;

        int end = fc.FindNumForNameFile("H_END", i, 0);
        if (end == -1)
        {
            CONS_Printf(" H_END missing in file '%s'.\n", fc.Name(i));
            continue; // no H_END, nothing accepted
        }

        for (; lump < end; lump++)
        {
            Texture *tex = textures.LoadLump(fc.FindNameForNum(lump), lump);
            if (!BuildMaterial(tex, doom_tex, TEX_wall, true) && !BuildMaterial(tex, flat_tex, TEX_floor, true))
            {
                CONS_Printf(" H_START texture '%8s' in file '%s' has no original, ignored.\n",
                            fc.FindNameForNum(lump),
                            fc.Name(i));
            }
        }
    }

    // ZDoom-compatible PK3 namespace directories.
    // textures/ and patches/ -> wall/ceiling textures (doom_tex)
    // flats/                 -> floor textures (flat_tex)
    // sprites/               -> sprites (sprite_tex)
    // hires/                 -> hi-res replacements, scale-preserving (like H_START)
    // Later-loaded files take precedence (iterate nwads-1 down to 0).
    {
        char stem[64]; // CACHE_NAME_LEN is 63

        for (i = nwads - 1; i >= 0; i--)
        {
            int nlumps = fc.GetNumLumps(i);
            int ns_count[7] = {0,0,0,0,0,0,0}; // per-namespace hit counts (indices 0..6 = TEXTURES..GRAPHICS)
            int replaced = 0, created = 0, hires_ok = 0, hires_miss = 0;

            // Multi-pass: pass 0 = root namespace, passes 1..MAX_DEPTH = filter entries
            // ordered by specificity (least specific first). This ensures that a more
            // specific filter path (e.g. filter/doom.id.doom1.registered/) always
            // overwrites a less specific one (e.g. filter/doom.id.doom1/).
            const int MAX_FILTER_DEPTH = 6;
            for (int pass = 0; pass <= MAX_FILTER_DEPTH; pass++)
            for (int item = 0; item < (int)nlumps; item++)
            {
                int lump = (i << 16) | item;
                const char *fullname = fc.FindNameForNum(lump);
                if (!fullname || !strchr(fullname, '/'))
                    continue; // not a path-based ZIP entry

                bool is_filter = (strncasecmp(fullname, "filter/", 7) == 0);
                if (pass == 0 && is_filter) continue;  // root pass: skip filter entries
                if (pass >  0 && !is_filter) continue; // filter passes: skip root entries

                if (is_filter)
                {
                    const char *gname = fullname + 7; // skip "filter/"
                    const char *slash = strchr(gname, '/');
                    if (!slash) continue; // bare "filter/GAME" with no further path
                    size_t glen = slash - gname;
                    if (!PK3_FilterMatchesGame(gname, glen)) continue;
                    if (PK3_FilterDepth(gname, glen) != pass) continue; // wrong depth for this pass
                }

                pk3_ns_t ns = PK3_GetNamespace(fullname, stem, sizeof(stem));
                if (ns == PK3_NONE)
                    continue;

                ns_count[ns - 1]++;

                Texture *tex = textures.LoadLump(stem, lump);
                if (!tex)
                    continue;

                switch (ns)
                {
                    case PK3_TEXTURES:
                    case PK3_PATCHES:
                    {
                        bool existed = textures.Exists(stem);
                        // ZDoom's textures/ is a universal namespace — it can replace
                        // walls, flats, or sprites. Route to whichever cache owns the name.
                        Material *m;
                        if (flat_tex.Find(stem))
                            m = BuildMaterial(tex, flat_tex, TEX_floor);
                        else if (sprite_tex.Find(stem))
                            m = BuildMaterial(tex, sprite_tex, TEX_sprite);
                        else
                            m = BuildMaterial(tex, doom_tex, TEX_wall);
                        if (m) { existed ? replaced++ : created++; num_textures++; }
                        break;
                    }
                    case PK3_FLATS:
                    {
                        bool existed = textures.Exists(stem);
                        Material *m = BuildMaterial(tex, flat_tex, TEX_floor);
                        if (m) { existed ? replaced++ : created++; num_textures++; }
                        break;
                    }
                    case PK3_SPRITES:
                    {
                        bool existed = textures.Exists(stem);
                        Material *m = BuildMaterial(tex, sprite_tex, TEX_sprite);
                        if (m) { existed ? replaced++ : created++; num_textures++; }
                        break;
                    }
                    case PK3_HIRES_SPRITES:
                    {
                        // Scale HD sprite to match original world dimensions.
                        Material *m = BuildMaterial(tex, sprite_tex, TEX_sprite, true);
                        if (m) { replaced++; }
                        else
                        {
                            // No original sprite yet (e.g. mod-only sprite) — add directly.
                            m = BuildMaterial(tex, sprite_tex, TEX_sprite, false);
                            if (m) { created++; num_textures++; }
                        }
                        break;
                    }
                    case PK3_HIRES:
                        if (BuildMaterial(tex, doom_tex, TEX_wall, true) ||
                            BuildMaterial(tex, flat_tex, TEX_floor, true) ||
                            BuildMaterial(tex, sprite_tex, TEX_sprite, true))
                            hires_ok++;
                        else
                            hires_miss++;
                        break;

                    case PK3_GRAPHICS:
                    {
                        // graphics/ and lumps/ namespaces: fullscreen images like TITLEPIC,
                        // HELP1, WIMAPs, etc. Register in lod_tex so materials.Get() finds them
                        // with the default TEX_lod mode (used by D_PageDrawer, menus, HUD).
                        bool existed = lod_tex.Find(stem) != NULL;
                        Material *m = BuildMaterial(tex, lod_tex, TEX_lod);
                        if (m) { existed ? replaced++ : created++; num_textures++; }
                        break;
                    }

                    default: break;
                }
            }

            if (ns_count[0] + ns_count[1] + ns_count[2] + ns_count[3] + ns_count[4] + ns_count[5] + ns_count[6] > 0)
            {
                CONS_Printf(" PK3 ns scan '%s': tex=%d pat=%d flat=%d spr=%d hires=%d hspr=%d gfx=%d"
                            " -> replaced=%d new=%d hires_ok=%d hires_miss=%d\n",
                            fc.Name(i),
                            ns_count[PK3_TEXTURES-1],      ns_count[PK3_PATCHES-1],
                            ns_count[PK3_FLATS-1],         ns_count[PK3_SPRITES-1],
                            ns_count[PK3_HIRES-1],         ns_count[PK3_HIRES_SPRITES-1],
                            ns_count[PK3_GRAPHICS-1],
                            replaced, created, hires_ok, hires_miss);
            }
        }
    }

    // GZDoom TEXTURES lump: "graphic 'DST' { patch 'SRC', 0, 0 {} }" aliases.
    // Scan for TEXTURES* lumps under matching filter/ paths (multi-depth ordered by specificity)
    // and parse them to create material aliases (e.g. TITLEPIC -> TITLED1 for registered doom).
    {
        // Collect (filterDepth, lumpnum) pairs for matching TEXTURES lumps, sorted by depth.
        // We'll process them in depth order so more-specific entries win.
        struct texdef_t { int depth; int lump; };
        std::vector<texdef_t> texlumps;

        for (i = nwads - 1; i >= 0; i--)
        {
            int nlumps = fc.GetNumLumps(i);
            for (int item = 0; item < nlumps; item++)
            {
                int lump = (i << 16) | item;
                const char *fullname = fc.FindNameForNum(lump);
                if (!fullname || !strchr(fullname, '/')) continue;
                if (strncasecmp(fullname, "filter/", 7) != 0) continue;

                const char *gname = fullname + 7;
                const char *slash = strchr(gname, '/');
                if (!slash) continue;
                size_t glen = slash - gname;
                if (!PK3_FilterMatchesGame(gname, glen)) continue;

                // The part after "filter/GAME/" should be "TEXTURES" or "TEXTURES.*"
                const char *rest = slash + 1;
                if (strncasecmp(rest, "TEXTURES", 8) != 0) continue;
                if (rest[8] != '\0' && rest[8] != '.' && rest[8] != '/') continue;
                // Must not have a subdirectory (it's a root-level lump in the filter dir)
                if (strchr(rest, '/')) continue;

                texdef_t td;
                td.depth = PK3_FilterDepth(gname, glen);
                td.lump  = lump;
                texlumps.push_back(td);
            }
        }

        // Sort by depth so least-specific entries are processed first (most-specific wins).
        std::sort(texlumps.begin(), texlumps.end(),
                  [](const texdef_t &a, const texdef_t &b) { return a.depth < b.depth; });

        for (const texdef_t &td : texlumps)
        {
            char *text = static_cast<char *>(fc.CacheLumpNum(td.lump, PU_STATIC, true));
            if (!text) continue;

            // Simple state-machine parser for the 'graphic "DST" { patch "SRC" ... }' format.
            const char *p = text;
            const char *end = text + strlen(text);

            auto skip_ws_comments = [&]() {
                while (p < end) {
                    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
                    if (p + 1 < end && p[0] == '/' && p[1] == '/') {
                        while (p < end && *p != '\n') p++;
                    } else break;
                }
            };

            auto read_token = [&](char *buf, size_t bsz) -> bool {
                skip_ws_comments();
                if (p >= end) return false;
                bool quoted = (*p == '"');
                if (quoted) p++;
                const char *s = p;
                while (p < end) {
                    if (quoted  && *p == '"') break;
                    if (!quoted && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ||
                                    *p == ',' || *p == '{' || *p == '}')) break;
                    p++;
                }
                size_t n = min((size_t)(p - s), bsz - 1);
                strncpy(buf, s, n); buf[n] = '\0'; strupr(buf);
                if (quoted && p < end) p++; // skip closing '"'
                return n > 0;
            };

            auto skip_past_block = [&]() {
                // skip to end of a { } block (handles nesting)
                int depth = 0;
                while (p < end) {
                    if (*p == '{') { depth++; }
                    else if (*p == '}') { if (--depth <= 0) { p++; break; } }
                    p++;
                }
            };

            char kw[64], dst[64], src[64];
            while (p < end) {
                if (!read_token(kw, sizeof(kw))) break;
                // We only care about 'graphic' entries.
                if (strcasecmp(kw, "graphic") != 0) {
                    // Skip name, optional numbers, then skip the { } block.
                    read_token(dst, sizeof(dst)); // consume name
                    skip_ws_comments();
                    while (p < end && *p != '{') p++; // skip to block
                    skip_past_block();
                    continue;
                }

                // graphic "DST", width, height { patch "SRC", x, y { } }
                if (!read_token(dst, sizeof(dst))) continue;
                skip_ws_comments();
                while (p < end && *p != '{') p++; // skip width, height (we don't need them)
                if (p >= end || *p != '{') continue;
                p++; // skip opening '{'

                // Read the first 'patch' inside the block.
                src[0] = '\0';
                int inner = 1;
                while (p < end && inner > 0) {
                    skip_ws_comments();
                    if (p >= end) break;
                    if (*p == '}') { inner--; p++; continue; }
                    if (!read_token(kw, sizeof(kw))) { p++; continue; }
                    if ((strcasecmp(kw, "patch") == 0 || strcasecmp(kw, "graphic") == 0) && src[0] == '\0') {
                        read_token(src, sizeof(src)); // the patch name
                        // skip x, y, and any nested { }
                        skip_ws_comments();
                        while (p < end && *p != '{' && *p != '}' && *p != '\n') p++;
                        if (p < end && *p == '{') skip_past_block();
                    } else {
                        // skip to end of line or block
                        while (p < end && *p != '\n' && *p != '{' && *p != '}') p++;
                        if (p < end && *p == '{') skip_past_block();
                    }
                }

                if (dst[0] == '\0' || src[0] == '\0') continue;

                // Find the source material (patch SRC must already be in lod_tex or doom_tex).
                Material *msrc = lod_tex.Find(src);
                if (!msrc) msrc = doom_tex.Find(src);
                if (!msrc || msrc->tex.empty()) continue;

                // Register DST in lod_tex pointing at SRC's texture.
                Material *mdst = lod_tex.Find(dst);
                if (!mdst) {
                    mdst = new Material(dst, msrc->tex[0].t);
                    lod_tex.Insert(mdst);
                    Register(mdst);
                } else {
                    Material::TextureRef &r = mdst->tex[0];
                    if (r.t) r.t->Release();
                    msrc->tex[0].t->AddRef();
                    r.t = msrc->tex[0].t;
                    r.xscale = 1; r.yscale = 1;
                    mdst->InitializeMaterial();
                }
                CONS_Printf(" GZDoom TEXTURES: graphic '%s' -> patch '%s'\n", dst, src);
                num_textures++;
            }

            Z_Free(text);
        }
    }

    // PK3 normal maps: materials/normalmaps/TEXNAME.png -> attach as tex[1] on matching Material.
    // Shader assignment happens later in R_Init() once a GL context exists.
    {
        int nm_attached = 0;
        for (i = nwads - 1; i >= 0; i--)
        {
            int nlumps = fc.GetNumLumps(i);
            for (int item = 0; item < nlumps; item++)
            {
                int lump = (i << 16) | item;
                const char *fullname = fc.FindNameForNum(lump);
                if (!fullname || !strchr(fullname, '/'))
                    continue;

                // Strip optional filter/GAME/ prefix
                const char *check = fullname;
                if (strncasecmp(check, "filter/", 7) == 0)
                {
                    check += 7;
                    const char *sl = strchr(check, '/');
                    if (!sl) continue;
                    check = sl + 1;
                }

                if (strncasecmp(check, "materials/normalmaps/", 21) != 0)
                    continue;

                // Extract uppercased stem (basename without extension)
                const char *base = check + 21;
                const char *sl2  = strrchr(base, '/');
                if (sl2) base = sl2 + 1;
                const char *dot  = strrchr(base, '.');
                size_t copy_len  = dot ? (size_t)(dot - base) : strlen(base);
                if (copy_len == 0 || copy_len >= 64)
                    continue;

                char nm_stem[64];
                strncpy(nm_stem, base, copy_len);
                nm_stem[copy_len] = '\0';
                strupr(nm_stem);

                // Find matching wall or flat material
                Material *m = doom_tex.Find(nm_stem);
                if (!m) m = flat_tex.Find(nm_stem);
                if (!m)
                    continue;
                if ((int)m->tex.size() >= 2)
                    continue; // already assigned

                // Load the normal map under a prefixed key to avoid cache collision
                char nmt_key[72];
                snprintf(nmt_key, sizeof(nmt_key), "NM_%s", nm_stem);
                Texture *nmt = textures.LoadLump(nmt_key, lump);
                if (!nmt)
                    continue;

                m->tex.resize(2);
                m->tex[1].t = nmt;
                nmt->AddRef(); // Material destructor calls Release() on all tex slots
                nm_attached++;
            }
        }
        if (nm_attached > 0)
            CONS_Printf(" Normal maps attached: %d materials.\n", nm_attached);
    }

    // GLDEFS: per-material PBR maps, scalars, and shader paths.
    // Runs after the normalmaps scan so GLDEFS paths take precedence for tex[1].
    ReadGLDEFS();

    Read_NTEXTURE(fc.FindNumForName("NTEXTURE"));
    Read_NTEXTURE(fc.FindNumForName("NSPRITES"));

    return num_textures;
}

// ---------------------------------------------------------------------------
// GLDEFS parser
// ---------------------------------------------------------------------------

/// Find a lump by its full path (case-insensitive). Searches the given file first,
/// then all other files from newest to oldest.
static int FindLumpByPath(const char *path, int preferred_file)
{
    int nfiles = (int)fc.Size();
    for (int pass = 0; pass < 2; pass++)
    {
        for (int fi = nfiles - 1; fi >= 0; fi--)
        {
            if ((pass == 0) != (fi == preferred_file))
                continue;
            int nlumps = fc.GetNumLumps(fi);
            for (int j = 0; j < nlumps; j++)
            {
                int lump = (fi << 16) | j;
                const char *n = fc.FindNameForNum(lump);
                if (n && strcasecmp(n, path) == 0)
                    return lump;
            }
        }
    }
    return -1;
}

/// Attach a texture from a GLDEFS path into a specific material slot.
/// Uses a prefixed cache key so it won't collide with diffuse textures.
static void AttachGLDEFSTex(Material *m, int slot, const char *path,
                             const char *prefix, int file_hint)
{
    int lump = FindLumpByPath(path, file_hint);
    if (lump < 0)
        return;

    // Grow the tex vector if needed
    if ((int)m->tex.size() <= slot)
        m->tex.resize(slot + 1);

    // Build a unique cache key: prefix + basename-without-extension
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    const char *dot = strrchr(base, '.');
    size_t blen = dot ? (size_t)(dot - base) : strlen(base);

    char key[128];
    snprintf(key, sizeof(key), "%s_", prefix);
    size_t plen = strlen(key);
    if (blen + plen >= sizeof(key))
        blen = sizeof(key) - plen - 1;
    strncpy(key + plen, base, blen);
    key[plen + blen] = '\0';
    strupr(key);

    Texture *t = textures.LoadLump(key, lump);
    if (!t)
        return;

    // Release old occupant if any
    if (m->tex[slot].t)
        m->tex[slot].t->Release();

    m->tex[slot].t = t;
    t->AddRef();
}

void material_cache_t::ReadGLDEFS()
{
    int total_mats = 0, total_maps = 0;

    for (int fi = (int)fc.Size() - 1; fi >= 0; fi--)
    {
        // The lump may be "GLDEFS" (WAD) or "GLDEFS.txt" / "GLDEFS.TXT" (PK3).
        // Search only within this specific file — no cross-file fallback.
        int lump = fc.FindNumForNameFile("GLDEFS", fi, 0);
        if (lump < 0)
        {
            int nlumps = (int)fc.GetNumLumps(fi);
            for (int j = 0; j < nlumps && lump < 0; j++)
            {
                int candidate = (fi << 16) | j;
                const char *n = fc.FindNameForNum(candidate);
                if (n && (strcasecmp(n, "GLDEFS.txt") == 0 || strcasecmp(n, "GLDEFS.TXT") == 0))
                    lump = candidate;
            }
        }
        if (lump < 0)
            continue;

        int len = fc.LumpLength(lump);
        char *buf = static_cast<char *>(Z_Malloc(len + 1, PU_DAVE, NULL));
        fc.ReadLump(lump, buf);
        buf[len] = '\0';

        // Simple line-by-line parser
        const char *p = buf;
        while (*p)
        {
            // Skip whitespace and blank lines
            while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
                p++;
            if (!*p)
                break;

            // Skip line comments
            if (p[0] == '/' && p[1] == '/')
            {
                while (*p && *p != '\n') p++;
                continue;
            }

            // Match "material texture" keyword
            if (strncasecmp(p, "material", 8) != 0)
            {
                while (*p && *p != '\n') p++;
                continue;
            }
            p += 8;
            while (*p == ' ' || *p == '\t') p++;
            if (strncasecmp(p, "texture", 7) != 0)
            {
                while (*p && *p != '\n') p++;
                continue;
            }
            p += 7;
            while (*p == ' ' || *p == '\t') p++;

            // Read quoted texture name
            if (*p != '"') { while (*p && *p != '\n') p++; continue; }
            p++;
            const char *name_start = p;
            while (*p && *p != '"' && *p != '\n') p++;
            if (*p != '"') continue;
            size_t name_len = p - name_start;
            p++; // skip closing quote

            char mat_name[64];
            if (name_len == 0 || name_len >= sizeof(mat_name)) continue;
            strncpy(mat_name, name_start, name_len);
            mat_name[name_len] = '\0';
            strupr(mat_name);

            // Find the material in any cache
            Material *m = doom_tex.Find(mat_name);
            if (!m) m = flat_tex.Find(mat_name);
            if (!m) m = sprite_tex.Find(mat_name);
            if (!m)
            {
                // Skip to closing brace
                while (*p && *p != '{') p++;
                if (*p == '{') { int depth=1; p++; while (*p && depth) { if (*p=='{') depth++; else if (*p=='}') depth--; p++; } }
                continue;
            }

            // Skip to opening brace
            while (*p && *p != '{') p++;
            if (*p != '{') continue;
            p++;

            total_mats++;

            // Parse key-value pairs inside the block
            while (*p)
            {
                while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
                if (*p == '}') { p++; break; }
                if (p[0] == '/' && p[1] == '/') { while (*p && *p != '\n') p++; continue; }

                // Read key token
                const char *key_start = p;
                while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
                size_t key_len = p - key_start;
                if (key_len == 0) continue;

                char key[32];
                if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
                strncpy(key, key_start, key_len);
                key[key_len] = '\0';

                while (*p == ' ' || *p == '\t') p++;

                // Dispatch on key
                if (strcasecmp(key, "normal") == 0 ||
                    strcasecmp(key, "metallic") == 0 ||
                    strcasecmp(key, "roughness") == 0 ||
                    strcasecmp(key, "ao") == 0 ||
                    strcasecmp(key, "specular") == 0 ||
                    strcasecmp(key, "brightmap") == 0 ||
                    strcasecmp(key, "shader") == 0)
                {
                    // Expect quoted path
                    if (*p != '"') { while (*p && *p != '\n') p++; continue; }
                    p++;
                    const char *val_start = p;
                    while (*p && *p != '"' && *p != '\n') p++;
                    size_t val_len = p - val_start;
                    if (*p == '"') p++;

                    char val[256];
                    if (val_len == 0 || val_len >= sizeof(val)) continue;
                    strncpy(val, val_start, val_len);
                    val[val_len] = '\0';

                    if (strcasecmp(key, "shader") == 0)
                    {
                        m->shader_lump = val;
                    }
                    else
                    {
                        static const struct { const char *key; int slot; const char *prefix; } map[] = {
                            {"normal",    1, "NM"},
                            {"metallic",  2, "MT"},
                            {"roughness", 3, "RG"},
                            {"ao",        4, "AO"},
                            {"specular",  5, "SP"},
                            {"brightmap", 6, "BM"},
                        };
                        for (int k = 0; k < 6; k++)
                        {
                            if (strcasecmp(key, map[k].key) == 0)
                            {
                                AttachGLDEFSTex(m, map[k].slot, val, map[k].prefix, fi);
                                total_maps++;
                                break;
                            }
                        }
                    }
                }
                else if (strcasecmp(key, "texture") == 0)
                {
                    // "texture displacement <quoted-path>"
                    const char *sub_start = p;
                    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
                    size_t sub_len = p - sub_start;
                    char sub[32];
                    if (sub_len >= sizeof(sub)) sub_len = sizeof(sub) - 1;
                    strncpy(sub, sub_start, sub_len);
                    sub[sub_len] = '\0';

                    while (*p == ' ' || *p == '\t') p++;
                    if (*p != '"') { while (*p && *p != '\n') p++; continue; }
                    p++;
                    const char *val_start = p;
                    while (*p && *p != '"' && *p != '\n') p++;
                    size_t val_len = p - val_start;
                    if (*p == '"') p++;

                    char val[256];
                    if (val_len == 0 || val_len >= sizeof(val)) continue;
                    strncpy(val, val_start, val_len);
                    val[val_len] = '\0';

                    if (strcasecmp(sub, "displacement") == 0)
                    {
                        AttachGLDEFSTex(m, 7, val, "DP", fi);
                        total_maps++;
                    }
                }
                else if (strcasecmp(key, "glossiness") == 0)
                {
                    m->glossiness = (float)atof(p);
                    while (*p && *p != '\n') p++;
                }
                else if (strcasecmp(key, "specularlevel") == 0)
                {
                    m->specularlevel = (float)atof(p);
                    while (*p && *p != '\n') p++;
                }
                else
                {
                    // Unknown key — skip rest of line
                    while (*p && *p != '\n') p++;
                }
            }
        }

        Z_Free(buf);
    }

    CONS_Printf(" GLDEFS: %d materials, %d extra maps loaded.\n", total_mats, total_maps);
}

void material_cache_t::ClearAllShaders()
{
    for (size_t mi = 0; mi < all_materials.size(); mi++)
        if (all_materials[mi])
            all_materials[mi]->shader = NULL;
}

void material_cache_t::AssignNormalMapShader(ShaderProg *prog)
{
    int count = 0;
    for (size_t mi = 0; mi < all_materials.size(); mi++)
    {
        Material *m = all_materials[mi];
        if (m && (int)m->tex.size() >= 2 && !m->shader)
        {
            m->shader = prog;
            count++;
        }
    }
    CONS_Printf("AssignNormalMapShader: %d materials assigned.\n", count);
}

//==================================================================
//    Colormaps
//==================================================================

/// extra boom colormaps
/*
int             num_extra_colormaps;
extracolormap_t extra_colormaps[MAXCOLORMAPS];
*/

/// the fadetable "cache"
static vector<fadetable_t *> fadetables;

/// lumplist for C_START - C_END pairs in wads
static int numcolormaplumps;
static lumplist_t *colormaplumps;

fadetable_t::~fadetable_t()
{
    if (colormap)
        Z_Free(colormap);
}

/// For custom, OpenGL-friendly colormaps
fadetable_t::fadetable_t()
{
    lump = -1;
    colormap = NULL;
}

/// For lump-based colormaps
fadetable_t::fadetable_t(int lumpnum)
{
    lump = lumpnum;

    int length = fc.LumpLength(lump);
    if (length != 34 * 256)
    {
        CONS_Printf("Unknown colormap size: %d bytes\n", length);
    }

    // (aligned on 8 bit for asm code) now 64k aligned for smokie...
    colormap = static_cast<lighttable_t *>(Z_MallocAlign(length, PU_STATIC, 0, 16)); // 8);
    fc.ReadLump(lump, colormap);

    // SoM: Added, we set all params of the colormap to normal because there
    // is no real way to tell how GL should handle a colormap lump anyway..
    // TODO try to analyze the colormap and somehow simulate it in GL...
    maskcolor = 0xffff;
    fadecolor = 0x0;
    maskamt = 0x0;
    fadestart = 0;
    fadeend = 33;
    fog = 0;
}

/// Returns the name of the fadetable
const char *R_ColormapNameForNum(const fadetable_t *p)
{
    if (!p)
        return "NONE";

    int temp = p->lump;
    if (temp == -1)
        return "INLEVEL";

    return fc.FindNameForNum(temp);
}

/// Called in R_Init, prepares the lightlevel colormaps and Boom extra colormaps.
void R_InitColormaps()
{
    if (devparm)
        CONS_Printf(" Loading colormaps.\n");

    // build the colormap lumplists (which record the locations of C_START and C_END in each wad)
    numcolormaplumps = 0;
    colormaplumps = NULL;

    int nwads = fc.Size();
    for (int file = 0; file < nwads; file++)
    {
        int start = fc.FindNumForNameFile("C_START", file, 0);
        if (start == -1)
            continue; // required

        int end = fc.FindNumForNameFile("C_END", file, 0);
        if (end == -1)
        {
            CONS_Printf("R_InitColormaps: C_START without C_END in file '%s'!\n", fc.Name(file));
            continue;
        }

        colormaplumps = static_cast<lumplist_t *>(
            realloc(colormaplumps, sizeof(lumplist_t) * (numcolormaplumps + 1)));
        colormaplumps[numcolormaplumps].wadfile = start >> 16;
        colormaplumps[numcolormaplumps].firstlump = (start & 0xFFFF) + 1; // after C_START
        colormaplumps[numcolormaplumps].numlumps = end - (start + 1);
        numcolormaplumps++;
    }

    // SoM: 3/30/2000: Init Boom colormaps.
    R_ClearColormaps();

    // Load in the basic lightlevel colormaps
    fadetables.push_back(new fadetable_t(fc.GetNumForName("COLORMAP")));
}

/// Clears out all extra colormaps.
void R_ClearColormaps()
{
    int n = fadetables.size();
    for (int i = 0; i < n; i++)
        delete fadetables[i];

    fadetables.clear();
}

/// Auxiliary Get func for fadetables
fadetable_t *R_GetFadetable(unsigned n)
{
    if (n < fadetables.size())
        return fadetables[n];
    else
        return NULL;
}

/// Tries to retrieve a colormap by name. Only checks stuff between C_START/C_END.
fadetable_t *R_FindColormap(const char *name)
{
    int lump = R_CheckNumForNameList(name, colormaplumps, numcolormaplumps);
    if (lump == -1)
        return NULL; // no such colormap

    int n = fadetables.size();
    for (int i = 0; i < n; i++)
        if (fadetables[i]->lump == lump)
            return fadetables[i];

    // not already cached, we must read it from WAD
    if (n == MAXCOLORMAPS)
        I_Error("R_ColormapNumForName: Too many colormaps!\n");

    fadetable_t *temp = new fadetable_t(lump);
    fadetables.push_back(temp);
    return temp;
}

/// Sets the default lighlevel colormaps used by the software renderer in this map.
/// All lumps are searched.
bool Map::R_SetFadetable(const char *name)
{
    int lump = fc.FindNumForName(name);
    if (lump < 0)
    {
        CONS_Printf("R_SetFadetable: Fadetable '%s' not found.\n", name);
        fadetable = fadetables[0]; // use COLORMAP
        return false;
    }

    int n = fadetables.size();
    for (int i = 0; i < n; i++)
        if (fadetables[i]->lump == lump)
        {
            fadetable = fadetables[i];
            return true;
        }

    // not already cached, we must read it from WAD
    if (n == MAXCOLORMAPS)
        I_Error("R_SetFadetable: Too many colormaps!\n");

    fadetable = new fadetable_t(lump);
    fadetables.push_back(fadetable);
    return true;
}

// Rounds off floating numbers and checks for 0 - 255 bounds
static int RoundUp(double number)
{
    if (number >= 255.0)
        return 255;
    if (number <= 0)
        return 0;

    if (int(number) <= (number - 0.5))
        return int(number) + 1;

    return int(number);
}

// R_CreateColormap
// This is a more GL friendly way of doing colormaps: Specify colormap
// data in a special linedef's texture areas and use that to generate
// custom colormaps at runtime. NOTE: For GL mode, we only need to color
// data and not the colormap data.

fadetable_t *R_CreateColormap(char *p1, char *p2, char *p3)
{
    int i, p;
    double cmaskr, cmaskg, cmaskb;
    double maskamt = 0, othermask = 0;
    unsigned int cr, cg, cb, ca;
    unsigned int maskcolor;
    int rgba;

    // TODO: Hurdler: check this entire function
    //  for now, full support of toptexture only

#define HEX2INT(x)                                                                                 \
    (x >= '0' && x <= '9'   ? x - '0'                                                              \
     : x >= 'a' && x <= 'f' ? x - 'a' + 10                                                         \
     : x >= 'A' && x <= 'F' ? x - 'A' + 10                                                         \
                            : 0)
#define ALPHA2INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : x >= 'A' && x <= 'Z' ? x - 'A' : 0)
    if (p1[0] == '#')
    {
        cmaskr = cr = ((HEX2INT(p1[1]) << 4) + HEX2INT(p1[2]));
        cmaskg = cg = ((HEX2INT(p1[3]) << 4) + HEX2INT(p1[4]));
        cmaskb = cb = ((HEX2INT(p1[5]) << 4) + HEX2INT(p1[6]));
        maskamt = ca = ALPHA2INT(p1[7]);

        // 24-bit color
        rgba = cr + (cg << 8) + (cb << 16) + (ca << 24);

        // Create a rough approximation of the color (a 16 bit color)
        maskcolor = (cb >> 3) + ((cg >> 2) << 5) + ((cr >> 3) << 11);

        maskamt /= 25.0;

        othermask = 1 - maskamt;
        maskamt /= 0xff;
        cmaskr *= maskamt;
        cmaskg *= maskamt;
        cmaskb *= maskamt;
    }
    else
    {
        cmaskr = 0xff;
        cmaskg = 0xff;
        cmaskb = 0xff;
        maskamt = 0;
        rgba = 0;
        maskcolor = ((0xff) >> 3) + (((0xff) >> 2) << 5) + (((0xff) >> 3) << 11);
    }
#undef ALPHA2INT

    unsigned int fadestart = 0, fadeend = 33, fadedist = 33;
    int fog = 0;

#define NUMFROMCHAR(c) (c >= '0' && c <= '9' ? c - '0' : 0)
    if (p2[0] == '#')
    {
        // SoM: Get parameters like, fadestart, fadeend, and the fogflag...
        fadestart = NUMFROMCHAR(p2[3]) + (NUMFROMCHAR(p2[2]) * 10);
        fadeend = NUMFROMCHAR(p2[5]) + (NUMFROMCHAR(p2[4]) * 10);
        if (fadestart > 32 || fadestart < 0)
            fadestart = 0;
        if (fadeend > 33 || fadeend < 1)
            fadeend = 33;
        fadedist = fadeend - fadestart;
        fog = NUMFROMCHAR(p2[1]) ? 1 : 0;
    }
#undef NUMFROMCHAR

    double cdestr, cdestg, cdestb;
    unsigned int fadecolor;
    if (p3[0] == '#')
    {
        cdestr = cr = ((HEX2INT(p3[1]) << 4) + HEX2INT(p3[2]));
        cdestg = cg = ((HEX2INT(p3[3]) << 4) + HEX2INT(p3[4]));
        cdestb = cb = ((HEX2INT(p3[5]) << 4) + HEX2INT(p3[6]));
        fadecolor = (((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11));
    }
    else
    {
        cdestr = 0;
        cdestg = 0;
        cdestb = 0;
        fadecolor = 0;
    }
#undef HEX2INT

    fadetable_t *f;

    // check if we already have created one like it
    int n = fadetables.size();
    for (i = 0; i < n; i++)
    {
        f = fadetables[i];
        if (f->lump == -1 && maskcolor == f->maskcolor && fadecolor == f->fadecolor &&
            maskamt == f->maskamt && fadestart == f->fadestart && fadeend == f->fadeend &&
            fog == f->fog && rgba == f->rgba)
            return f;
    }

    // nope, we have to create it now

    if (n == MAXCOLORMAPS)
        I_Error("R_CreateColormap: Too many colormaps!\n");

    f = new fadetable_t();
    fadetables.push_back(f);

    f->maskcolor = maskcolor;
    f->fadecolor = fadecolor;
    f->maskamt = maskamt;
    f->fadestart = fadestart;
    f->fadeend = fadeend;
    f->fog = fog;
    f->rgba = rgba;

    // OpenGL renderer does not need the colormap part
    if (rendermode == render_soft)
    {
        double deltas[256][3], cmap[256][3];

        for (i = 0; i < 256; i++)
        {
            double r, g, b;
            r = vid.palette[i].r;
            g = vid.palette[i].g;
            b = vid.palette[i].b;
            double cbrightness = sqrt((r * r) + (g * g) + (b * b));

            cmap[i][0] = (cbrightness * cmaskr) + (r * othermask);
            if (cmap[i][0] > 255.0)
                cmap[i][0] = 255.0;
            deltas[i][0] = (cmap[i][0] - cdestr) / (double)fadedist;

            cmap[i][1] = (cbrightness * cmaskg) + (g * othermask);
            if (cmap[i][1] > 255.0)
                cmap[i][1] = 255.0;
            deltas[i][1] = (cmap[i][1] - cdestg) / (double)fadedist;

            cmap[i][2] = (cbrightness * cmaskb) + (b * othermask);
            if (cmap[i][2] > 255.0)
                cmap[i][2] = 255.0;
            deltas[i][2] = (cmap[i][2] - cdestb) / (double)fadedist;
        }

        char *colormap_p = static_cast<char *>(Z_MallocAlign((256 * 34) + 1, PU_LEVEL, 0, 8));
        f->colormap = reinterpret_cast<lighttable_t *>(colormap_p);

        for (p = 0; p < 34; p++)
        {
            for (i = 0; i < 256; i++)
            {
                *colormap_p =
                    NearestColor(RoundUp(cmap[i][0]), RoundUp(cmap[i][1]), RoundUp(cmap[i][2]));
                colormap_p++;

                if ((unsigned int)p < fadestart)
                    continue;

#define ABS2(x) ((x) < 0 ? -(x) : (x))

                if (ABS2(cmap[i][0] - cdestr) > ABS2(deltas[i][0]))
                    cmap[i][0] -= deltas[i][0];
                else
                    cmap[i][0] = cdestr;

                if (ABS2(cmap[i][1] - cdestg) > ABS2(deltas[i][1]))
                    cmap[i][1] -= deltas[i][1];
                else
                    cmap[i][1] = cdestg;

                if (ABS2(cmap[i][2] - cdestb) > ABS2(deltas[i][1]))
                    cmap[i][2] -= deltas[i][2];
                else
                    cmap[i][2] = cdestb;

#undef ABS2
            }
        }
    }

    return f;
}

//  build a table for quick conversion from 8bpp to 15bpp
static int makecol15(int r, int g, int b)
{
    return (((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3)));
}

void R_Init8to16()
{
    int i;
    byte *palette = static_cast<byte *>(fc.CacheLumpName("PLAYPAL", PU_DAVE));

    for (i = 0; i < 256; i++)
    {
        // doom PLAYPAL are 8 bit values
        color8to16[i] = makecol15(palette[0], palette[1], palette[2]);
        palette += 3;
    }
    Z_Free(palette);

    // test a big colormap
    hicolormaps = static_cast<short int *>(Z_Malloc(32768 /**34*/, PU_STATIC, 0));
    for (i = 0; i < 16384; i++)
        hicolormaps[i] = i << 1;
}

//==================================================================
//    Translucency tables
//==================================================================

static int num_transtables;
int transtable_lumps[MAXTRANSTABLES]; // i'm so lazy
byte *transtables[MAXTRANSTABLES];

/// Tries to retrieve a transmap by name. Returns transtable number, or -1 if unsuccesful.
static int R_TransmapNumForName(const char *name)
{
    int lump = fc.FindNumForName(name);
    if (lump >= 0 && fc.LumpLength(lump) == tr_size)
    {
        for (int i = 0; i < num_transtables; i++)
            if (transtable_lumps[i] == lump)
                return i;

        if (num_transtables == MAXTRANSTABLES)
            I_Error("R_TransmapNumForName: Too many transmaps!\n");

        int n = num_transtables++;
        transtables[n] = static_cast<byte *>(Z_MallocAlign(tr_size, PU_STATIC, 0, 16));
        fc.ReadLump(lump, transtables[n]);
        return n;
    }
    return -1;
}

void R_InitTranslucencyTables()
{
    CONS_Printf("R_InitTranslucencyTables: start\n");
    if (devparm)
        CONS_Printf(" Loading translucency tables.\n");

    // NOTE: the translation tables MUST BE aligned on 64k for the asm optimised code
    //       (in other words, transtables pointer low word is 0)
    int i;

    for (i = 0; i < MAXTRANSTABLES; i++)
    {
        transtable_lumps[i] = -1;
        transtables[i] = NULL;
    }

    transtables[0] = static_cast<byte *>(Z_MallocAlign(tr_size, PU_STATIC, 0, 16));

    // load in translucency tables

    // first transtable
    // check for the Boom default transtable lump
    int lump = fc.FindNumForName("TRANMAP");
    if (lump >= 0)
        fc.ReadLump(lump, transtables[0]);
    else if (game.mode >= gm_heretic)
    {
        lump = fc.FindNumForName("TINTTAB");
        if (lump >= 0)
            fc.ReadLump(lump, transtables[0]);
    }
    else
    {
        lump = fc.FindNumForName("TRANSMED"); // in legacy.wad
        if (lump >= 0)
            fc.ReadLump(lump, transtables[0]);
    }

    CONS_Printf("R_InitTranslucencyTables: loaded first table\n");

    if (game.mode >= gm_heretic)
    {
        // all the basic transmaps are the same
        for (i = 1; i < 5; i++)
            transtables[i] = transtables[0];
    }
    else
    {
        // we can use the transmaps in legacy.wad - but check if they exist
        for (i = 1; i < 5; i++)
            transtables[i] = static_cast<byte *>(Z_MallocAlign(tr_size, PU_STATIC, 0, 16));

        int lump;
        lump = fc.FindNumForName("TRANSMOR");
        if (lump >= 0)
            fc.ReadLump(lump, transtables[1]);
        lump = fc.FindNumForName("TRANSHI");
        if (lump >= 0)
            fc.ReadLump(lump, transtables[2]);
        lump = fc.FindNumForName("TRANSFIR");
        if (lump >= 0)
            fc.ReadLump(lump, transtables[3]);
        lump = fc.FindNumForName("TRANSFX1");
        if (lump >= 0)
            fc.ReadLump(lump, transtables[4]);
    }

    CONS_Printf("R_InitTranslucencyTables: done\n");
    num_transtables = 5;

    // Compose a default linear filter map based on PLAYPAL.
    /*
    // Thanks to TeamTNT for prBoom sources!
    if (false)
      {
        // filter weights
        float w1 = 0.66;
        float w2 = 1 - w1;

        int i, j;
        byte *tp = transtables[0];

        for (i=0; i<256; i++)
      {
        float r2 = vid.palette[i].red   * w2;
        float g2 = vid.palette[i].green * w2;
        float b2 = vid.palette[i].blue  * w2;

        for (j=0; j<256; j++, tp++)
          {
            byte r = vid.palette[j].red   * w1 + r2;
            byte g = vid.palette[j].green * w1 + g2;
            byte b = vid.palette[j].blue  * w1 + b2;

            *tp = NearestColor(r, g, b);
          }
      }
      }
    */
}

//
// Preloads all relevant graphics for the Map asynchronously.
void Map::PrecacheMap()
{
    // Skip if async loading is disabled
    if (!cv_async_loading.value)
        return;

    // Use a set to avoid duplicate queueing
    std::set<std::string> queuedTextures;
    std::set<std::string> queuedMaterials;

    auto queueTexture = [&](const char* name) {
        if (!name || name[0] == '-')  // Skip "no texture" markers
            return;
        std::string texname(name);
        // Convert to uppercase for consistency
        for (auto& c : texname)
            c = toupper(c);
        if (queuedTextures.insert(texname).second)
        {
            // New texture - queue it
            AsyncCache_QueueTexture(texname.c_str(), PRIORITY_NORMAL);
        }
    };

    auto queueMaterial = [&](Material* mat) {
        if (!mat)
            return;
        const char* name = mat->GetName();
        if (!name || name[0] == '-')
            return;
        std::string matname(name);
        if (queuedMaterials.insert(matname).second)
        {
            // New material - queue it
            AsyncCache_QueueMaterial(matname.c_str(), PRIORITY_NORMAL);
        }
    };

    // Precache floor/ceiling textures from sectors
    for (int i = 0; i < numsectors; i++)
    {
        sector_t* sec = &sectors[i];
        queueMaterial(sec->floorpic);
        queueMaterial(sec->ceilingpic);
    }

    // Precache wall textures from linedefs
    for (int i = 0; i < numlines; i++)
    {
        line_t* line = &lines[i];
        
        // Precache from sidedefs
        for (int side = 0; side < 2; side++)
        {
            if (line->sideptr[side])
            {
                side_t* sd = line->sideptr[side];
                queueMaterial(sd->toptexture);
                queueMaterial(sd->midtexture);
                queueMaterial(sd->bottomtexture);
            }
        }
    }

    // Precache sky texture
    if (skytexture)
        queueMaterial(skytexture);

    // Queue the async processing
    // The actual loading happens in the background thread
    // and completed items are processed in the main loop
    
    CONS_Printf("Async precache queued: %d textures, %d materials\n",
                (int)queuedTextures.size(), (int)queuedMaterials.size());
}

// ============================================================================
// Async Texture/Material Loading
// ============================================================================

#include "z_ascache.h"

void R_QueueTextureAsync(const char *name, int priority)
{
    if (!name)
        return;
    
    AsyncCacheLoader::Instance().QueueTexture(name, priority);
}

void R_QueueMaterialAsync(const char *name, int priority)
{
    if (!name)
        return;
    
    AsyncCacheLoader::Instance().QueueMaterial(name, priority);
}

void R_ProcessAsyncTextures()
{
    // Completions are processed once per frame from d_main.cpp via AsyncCache_ProcessCompletions()
}
