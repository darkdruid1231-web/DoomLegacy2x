// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: oglrenderer.cpp 536 2009-06-29 06:46:13Z smite-meister $
//
// Copyright (C) 2006-2007 by DooM Legacy Team.
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
/// \brief OpenGL renderer.

#define GL_GLEXT_PROTOTYPES 1

#if defined(_WIN32) || defined(__MINGW32__)
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#include <SDL/SDL_opengl.h>
#endif

// Compatibility macros for SDL1/SDL2 screen dimension access
#ifdef SDL2
// SDL2 uses SDL_GetWindowSize which takes two pointers
// We need to provide wrapper functions
inline int SDL_GetWindowWidth_wrapper(SDL_Window *w) { int width, height; SDL_GetWindowSize(w, &width, &height); return width; }
inline int SDL_GetWindowHeight_wrapper(SDL_Window *w) { int width, height; SDL_GetWindowSize(w, &width, &height); return height; }
#define SDL_GetWindowWidth SDL_GetWindowWidth_wrapper
#define SDL_GetWindowHeight SDL_GetWindowHeight_wrapper
#define SCREEN_W(w) SDL_GetWindowWidth(w)
#define SCREEN_H(h) SDL_GetWindowHeight(h)
#else
#define SCREEN_W(s) ((s)->w)
#define SCREEN_H(s) ((s)->h)
#endif

#include "command.h"
#include "cvars.h"
#include "doomdef.h"

#include "g_actor.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_pawn.h"
#include "g_player.h"
#include "p_maputl.h"

#include "hardware/oglhelpers.hpp"
#include "hardware/oglrenderer.hpp"
#include "hardware/oglshaders.h"
#include "hardware/hw_postfx.h"
#include "hardware/hw_lightdefs.h"

#include "am_map.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_presentation.h"
#include "r_sprite.h"
#include "screen.h"
#include "tables.h"
#include "v_video.h"
#include "w_wad.h" // Need file cache to get playpal.
#include "z_zone.h"

#include <cmath>

extern trace_t trace;

void MD3_InitNormLookup();

// CVar definitions (declared extern in cvars.h; hwr_render.cpp is not compiled).
// Initialize value=1 so features are active before Reg() is called.
consvar_t cv_grstaticlighting = {"gr_staticlighting", "On", CV_SAVE, CV_OnOff, NULL, 1};
consvar_t cv_grshadows        = {"gr_shadows",        "On", CV_SAVE, CV_OnOff, NULL, 1};
consvar_t cv_grcoronas        = {"gr_coronas",        "On", CV_SAVE, CV_OnOff, NULL, 1};
consvar_t cv_grcoronasize     = {"gr_coronasize",    "1",   CV_SAVE, NULL, NULL, 1};
consvar_t cv_monball_light    = {"monball_light",     "On", CV_SAVE, CV_OnOff, NULL, 1};

/// Convert Doom light level (0-255) to OpenGL color component (0.0-1.0).
/// Uses a non-linear curve below 192 to match the software renderer's dark look.
static float LightLevelToFloat(int lightlevel)
{
    if (lightlevel < 0)   lightlevel = 0;
    if (lightlevel > 255) lightlevel = 255;
    float f = lightlevel / 255.0f;
    if (lightlevel < 192)
        f = (192.0f - (192.0f - lightlevel) * 1.87f) / 255.0f;
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return f;
}

// Dynamic light emitter table is now managed by hw_lightdefs.cpp (Phase B).
// InitLightDefs() populates the table at startup; ParseLightDefs() extends it.

// ---------------------------------------------------------------------------
// Sprite-based dynamic light table.
// Maps projectile/effect sprite numbers to light properties.
// Values ported from legacy_one/trunk/src/p_lights.c sprite_light[] table.
// ---------------------------------------------------------------------------
struct SpriteLightDef {
    spritenum_t sprite;
    float r, g, b;
    float radius;
};

static const SpriteLightDef spriteLightTable[] = {
    // Doom projectiles
    { SPR_MISL, 0.97f, 0.97f, 0.13f, 120.0f },  // Rocket: yellow-white
    { SPR_PLSS, 0.13f, 0.31f, 1.00f,  80.0f },  // Plasma shot: electric blue
    { SPR_BFS1, 0.13f, 1.00f, 0.13f, 200.0f },  // BFG ball: green
    { SPR_BAL1, 1.00f, 0.31f, 0.06f, 100.0f },  // Imp fireball: red-orange
    { SPR_BAL2, 1.00f, 0.31f, 0.06f, 100.0f },  // Cacodemon fireball: red-orange
    { SPR_FATB, 1.00f, 0.40f, 0.06f, 100.0f },  // Mancubus fireball: orange
    { SPR_APLS, 0.13f, 0.31f, 1.00f,  80.0f },  // Arachnotron plasma: blue
    { SPR_BAL7, 0.94f, 0.60f, 0.10f,  90.0f },  // Revenant tracer: orange-yellow
    // Doom explosions / impact flashes (larger radius)
    { SPR_BFE1, 0.13f, 1.00f, 0.13f, 300.0f },  // BFG explosion: bright green
    { SPR_BFE2, 0.13f, 1.00f, 0.13f, 200.0f },  // BFG secondary flash: green
    { SPR_PLSE, 0.13f, 0.31f, 1.00f, 100.0f },  // Plasma impact: blue
    { SPR_APBX, 0.13f, 0.31f, 1.00f, 100.0f },  // Arachnotron impact: blue
    { SPR_FBXP, 1.00f, 0.60f, 0.10f, 200.0f },  // Fireball explosion: orange
};
static const int NUM_SPRITE_LIGHTS = sizeof(spriteLightTable) / sizeof(spriteLightTable[0]);

/// Collect all dynamic lights for this frame: static decorative emitters,
/// sprite-based projectile lights, and a point light at the camera when firing.
void OGLRenderer::CollectDynamicLights(Actor *pov)
{
    framelights.clear();
    if (!mp)
        return;

    // Player weapon flash: add a white point light at the camera.
    PlayerPawn *ppawn = dynamic_cast<PlayerPawn *>(pov);
    if (ppawn && ppawn->extralight > 0)
    {
        FrameLight fl;
        fl.x = x;  fl.y = y;  fl.z = z;
        fl.r = 1.0f;  fl.g = 1.0f;  fl.b = 1.0f;
        fl.radius = ppawn->extralight * 96.0f;
        framelights.push_back(fl);
    }

    // Walk every sector's thing list for static emitters and projectile lights.
    for (int i = 0; i < mp->numsectors; i++)
    {
        sector_t *sec = &mp->sectors[i];
        for (Actor *th = sec->thinglist; th; th = th->snext)
        {
            DActor *da = dynamic_cast<DActor *>(th);
            if (!da)
                continue;

            // --- Static decorative emitters (data-driven via hw_lightdefs) ---
            FrameLight fl;
            if (GetActorLight(da, fl))
            {
                framelights.push_back(fl);
                continue;
            }

            // --- Sprite-based lights (projectiles and explosions) ---
            // Any actor whose current sprite matches the table emits light.
            // Gated by cv_monball_light so players can disable it (legacy_one parity).
            if (!cv_monball_light.value)
                continue;
            if (!da->state)
                continue;
            spritenum_t spr = da->state->sprite;
            for (int j = 0; j < NUM_SPRITE_LIGHTS; j++)
            {
                if (spr == spriteLightTable[j].sprite)
                {
                    FrameLight fl;
                    fl.x = da->pos.x.Float();
                    fl.y = da->pos.y.Float();
                    // Vertical center of actor.
                    fl.z = (da->pos.z + (da->height >> 1)).Float();
                    fl.r = spriteLightTable[j].r;
                    fl.g = spriteLightTable[j].g;
                    fl.b = spriteLightTable[j].b;
                    fl.radius = spriteLightTable[j].radius;
                    framelights.push_back(fl);
                    break;
                }
            }
        }
    }
}

/// Accumulate dynamic light RGB contribution at world point (px, py, pz).
/// Results are added (not clamped) into r, g, b — caller clamps to [0,1].
void OGLRenderer::AccumDynLight(float px, float py, float pz,
                                float &r, float &g, float &b) const
{
    for (size_t i = 0; i < framelights.size(); i++)
    {
        const FrameLight &fl = framelights[i];
        float dx = px - fl.x, dy = py - fl.y, dz = pz - fl.z;
        float dist2 = dx*dx + dy*dy + dz*dz;
        if (dist2 >= fl.radius * fl.radius)
            continue;
        float atten = 1.0f - sqrtf(dist2) / fl.radius;
        r += fl.r * atten;
        g += fl.g * atten;
        b += fl.b * atten;
    }
}

/// Transform framelights world positions to eye-space and upload to the shader.
/// Called once per surface when a GLSL shader is active — replaces CPU AccumDynLight.
void OGLRenderer::SetShaderDynamicLights(ShaderProg *prog) const
{
    int count = (int)framelights.size();
    if (count > MAX_SHADER_LIGHTS) count = MAX_SHADER_LIGHTS;
    if (count == 0)
    {
        prog->SetDynamicLights(NULL, NULL, 0);
        return;
    }

    // Read current ModelView matrix to transform world → eye space.
    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);

    float eyePos[MAX_SHADER_LIGHTS * 4];
    float colors[MAX_SHADER_LIGHTS * 3];

    for (int i = 0; i < count; i++)
    {
        const FrameLight &fl = framelights[i];
        float wx = fl.x, wy = fl.y, wz = fl.z;
        // Column-major MV multiply: eye = MV * [wx wy wz 1]
        eyePos[i*4+0] = mv[0]*wx + mv[4]*wy + mv[8] *wz + mv[12];
        eyePos[i*4+1] = mv[1]*wx + mv[5]*wy + mv[9] *wz + mv[13];
        eyePos[i*4+2] = mv[2]*wx + mv[6]*wy + mv[10]*wz + mv[14];
        eyePos[i*4+3] = fl.radius;
        colors[i*3+0] = fl.r;
        colors[i*3+1] = fl.g;
        colors[i*3+2] = fl.b;
    }
    prog->SetDynamicLights(eyePos, colors, count);
}

// ---------------------------------------------------------------------------
// Blob shadow system (Phase 4)
// ---------------------------------------------------------------------------

/// Build a 64×64 circular gradient texture used for all blob shadows.
/// Alpha is highest at the centre, zero at the edges.  Called once from InitGLState.
void OGLRenderer::InitShadowTexture()
{
    const int SIZE = 64;
    unsigned char data[SIZE * SIZE * 4];
    const float cx = SIZE * 0.5f, cy = SIZE * 0.5f;
    const float maxDist = SIZE * 0.5f;

    for (int y = 0; y < SIZE; y++)
    {
        for (int x = 0; x < SIZE; x++)
        {
            float dx = x - cx, dy = y - cy;
            float dist = sqrtf(dx*dx + dy*dy);
            float a = 1.0f - dist / maxDist;
            if (a < 0.0f) a = 0.0f;
            a = a * a;  // quadratic falloff — softer edge
            int i = (y * SIZE + x) * 4;
            data[i+0] = 0;
            data[i+1] = 0;
            data[i+2] = 0;
            data[i+3] = (unsigned char)(a * 200);  // peak alpha ~78%
        }
    }

    glGenTextures(1, &shadowTex);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE, SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/// Draw a blob shadow on the floor under `thing`.
/// Only monsters and players cast shadows (Doom64 style — items do not).
/// The shadow fades as the actor rises above the floor, giving a height cue.
void OGLRenderer::DrawBlobShadow(Actor *thing) const
{
    if (!shadowTex)
        return;

    // Skip projectiles, out-of-sector actors, and invisible actors.
    if (thing->flags & (MF_MISSILE | MF_NOSECTOR))
        return;

    // Only monsters and players cast shadows — items and decorations do not.
    bool isMonster = (thing->flags & MF_COUNTKILL) != 0;
    bool isPlayer  = (dynamic_cast<const PlayerPawn *>(thing) != NULL);
    if (!isMonster && !isPlayer)
        return;

    float floorZ      = thing->floorz.Float();
    float actorZ      = thing->pos.z.Float();
    float heightAbove = actorZ - floorZ;

    // Fade and shrink as the actor rises (full at floor, gone at 192 units up).
    float fade = 1.0f - heightAbove / 192.0f;
    if (fade <= 0.0f) return;
    if (fade > 1.0f)  fade = 1.0f;

    // Shadow radius matches the actor's collision footprint exactly (1:1).
    float shadowRadius = thing->radius.Float() * fade;
    if (shadowRadius < 4.0f)
        return;

    float ax = thing->pos.x.Float();
    float ay = thing->pos.y.Float();
    float az = floorZ + 0.5f;  // just above the floor to avoid z-fighting

    // Phase E: find the nearest dynamic light within 256 units.
    // Brighter/closer lights cast a slightly directional shadow and reduce
    // the blob's opacity (the floor is already lit, so the shadow is less noticeable).
    float shadowOpacity = fade;
    float offsetX = 0.0f, offsetY = 0.0f;
    {
        float bestDist2 = 256.0f * 256.0f;
        const FrameLight *nearest = NULL;
        for (size_t li = 0; li < framelights.size(); li++)
        {
            const FrameLight &fl = framelights[li];
            float dx = ax - fl.x, dy = ay - fl.y;
            float d2 = dx*dx + dy*dy;
            if (d2 < bestDist2) { bestDist2 = d2; nearest = &fl; }
        }
        if (nearest)
        {
            float atten = 1.0f - sqrtf(bestDist2) / 256.0f;
            float brightness = (nearest->r * 0.299f + nearest->g * 0.587f + nearest->b * 0.114f)
                               * atten;
            // Bright light → lighter shadow (floor is already lit)
            shadowOpacity *= 1.0f - brightness * 0.5f;

            // Offset shadow slightly away from light source (directional shadow).
            float ldx = ax - nearest->x, ldy = ay - nearest->y;
            float llen = sqrtf(ldx*ldx + ldy*ldy);
            if (llen > 1.0f)
            {
                float shift = shadowRadius * 0.25f * atten;
                offsetX = (ldx / llen) * shift;
                offsetY = (ldy / llen) * shift;
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_ALPHA_TEST);   // gradient would be clipped at alpha < 0.5
    glDepthMask(GL_FALSE);      // read depth but don't write it

    float cx = ax + offsetX, cy = ay + offsetY;
    glColor4f(0.0f, 0.0f, 0.0f, shadowOpacity);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(cx - shadowRadius, cy - shadowRadius, az);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(cx + shadowRadius, cy - shadowRadius, az);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(cx + shadowRadius, cy + shadowRadius, az);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(cx - shadowRadius, cy + shadowRadius, az);
    glEnd();
}

/// Draw blob shadows for every actor in the map in a single pass after all
/// floor geometry has been rendered.  The depth test clips each shadow quad
/// to whichever floor faces it overlaps, so shadows span subsector boundaries.
void OGLRenderer::DrawAllBlobShadows() const
{
    if (!mp || !shadowTex)
        return;

    // Set up shadow state once for the whole pass.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);     // ≤ so the shadow sits on the floor surface

    for (int i = 0; i < mp->numsectors; i++)
    {
        sector_t *sec = &mp->sectors[i];
        for (Actor *th = sec->thinglist; th; th = th->snext)
            if (!(th->flags2 & MF2_DONTDRAW))
                DrawBlobShadow(th);
    }

    glPopAttrib();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

/// Draw additive corona halos at each dynamic light position.
/// Coronas use the same circular gradient texture as blob shadows but are
/// rendered with additive blending so they glow on top of the scene.
/// Camera-aligned billboard quads are built from the modelview matrix.
void OGLRenderer::DrawAllCoronas() const
{
    if (framelights.empty() || !shadowTex)
        return;

    // Extract camera right and up vectors from the current modelview matrix.
    // In OpenGL's column-major layout the first row of the 3x3 rotation
    // sub-matrix lives at mv[0], mv[4], mv[8] (right) and
    // mv[1], mv[5], mv[9] (up).
    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    float rx = mv[0], ry = mv[4], rz = mv[8];
    float ux = mv[1], uy = mv[5], uz = mv[9];

    // Corona scale: integer 1–10, where 1 = default size.
    float coronaScale = (float)cv_grcoronasize.value;
    if (coronaScale <= 0.0f) coronaScale = 1.0f;

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive — glow on top of geometry
    glDepthMask(GL_FALSE);               // don't write to depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_ALPHA_TEST);

    for (size_t i = 0; i < framelights.size(); i++)
    {
        const FrameLight &fl = framelights[i];
        // Corona size is proportional to light radius, scaled by the CVar.
        float size = fl.radius * 0.15f * coronaScale;

        float lx = fl.x, ly = fl.y, lz = fl.z;

        // Build a camera-facing quad centered at the light position.
        glColor4f(fl.r, fl.g, fl.b, 0.5f);
        glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(lx - rx*size - ux*size, ly - ry*size - uy*size, lz - rz*size - uz*size);
        glTexCoord2f(1,0); glVertex3f(lx + rx*size - ux*size, ly + ry*size - uy*size, lz + rz*size - uz*size);
        glTexCoord2f(1,1); glVertex3f(lx + rx*size + ux*size, ly + ry*size + uy*size, lz + rz*size + uz*size);
        glTexCoord2f(0,1); glVertex3f(lx - rx*size + ux*size, ly - ry*size + uy*size, lz - rz*size + uz*size);
        glEnd();
    }

    glPopAttrib();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// Debug: wireframe mode for diagnosing floor/ceiling issues
static bool r_wireframe = false;

/*!
  \defgroup g_opengl OpenGL renderer

  Definitions:
    s: physical screen dimensions
    p: physical pixel dimensions
    r: screen resolution in pixels
    Screen aspect ratio:     a_s := w_s/h_s
    Pixel aspect ratio:      a_p := w_p/h_p
    Resolution aspect ratio: a_r := w_r/h_r

    Fundamental equalities:  w_s = w_r*w_p, h_s = h_r*h_p

    From these we get   a_s = a_p * a_r


  Doom was originally meant to be rendered on a CRT display with a_s = 4/3 aspect ratio and 320x200
  resolution, which gives a_r = 320/200 = 1.6 = 16/10 and a_p = 5/6. Hence the pixels are not
  supposed to be square, but a little stretched in the vertical direction.

  Modern display equipment almost invariably has square pixels, i.e. a_p = 1. A "standard" display
  still has a_s = 4/3, whereas a widescreen display has a_s = 16/10.

  And herein we have a problem. If we want to draw a 2D Doom graphic on a modern screen, we'll have
  to scale it in the y direction by a factor of 6/5 or it'll look squashed. However, if we want to
  use nearest-pixel sampling for a crisp, sharp image, the scaling factors in both x and y
  directions should be integers to avoid visual artefacts. The lowest resolution where this can be
  perfectly accomplished even for full-screen images is 1600x1200 (sx = 5, sy = 6). For smaller
  images, e.g. menu graphics, we could use the perfect scaling but draw the images closer together
  to fit them on screen? Additionally, with a widescreen display 1/6 of the screen area is unused
  (the sides). NOTE: At the moment we do no such y-scaling, so the 2D images ARE squashed. TODO 3D?

  The 3D view does not cause such problems, because all the textures need to be scaled anyway
  depending on the viewing angle and distance.
*/

OGLRenderer::OGLRenderer()
{
    workinggl = false;
    glversion = 0;
    screen = NULL;
#ifdef SDL2
    glCtx = NULL;
#endif
    curssec = NULL;
    shadowTex = 0;

    palette = static_cast<RGB_t *>(fc.CacheLumpName("PLAYPAL", PU_STATIC));

    mp = NULL;

    x = y = z = 0.0;
    theta = phi = 0.0;
    viewportw = viewporth = 0;

    fov = 90.0;

    hudar = 4.0 / 3.0; // Basic DOOM hud takes the full screen.
    screenar = 1.0;    // Pick a default value, any default value.
    viewportar = 1.0;

    // Initially we are at 2D mode.
    consolemode = true;

    InitLumLut();
    MD3_InitNormLookup();

    CONS_Printf("New OpenGL renderer created.\n");
}

OGLRenderer::~OGLRenderer()
{
    CONS_Printf("Closing OpenGL renderer.\n");
    Z_Free(palette);
#ifdef SDL2
    if (glCtx)
    {
        SDL_GL_DeleteContext(glCtx);
        glCtx = NULL;
    }
    if (screen)
    {
        SDL_DestroyWindow(screen);
        screen = NULL;
    }
#endif
}

/// Sets up those GL states that never change during rendering.
void OGLRenderer::InitGLState()
{
    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);

    // GL_ALPHA_TEST gets rid of "black boxes" around sprites. Might also speed up rendering a bit?
    glAlphaFunc(GL_GEQUAL, 0.5); // 0.5 is an optimal value for linear magfiltering
    glEnable(GL_ALPHA_TEST);

    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);

    // Sector lighting: disable hardware lighting and use glColor4f + GL_MODULATE.
    // Each surface sets its own brightness via glColor4f before drawing.
    glDisable(GL_LIGHTING);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1.0, 1.0, 1.0, 1.0);

    // red aiming dot parameters
    glEnable(GL_POINT_SMOOTH);
    glPointSize(8.0);
    glPointParameterf(GL_POINT_SIZE_MIN, 2.0);
    glPointParameterf(GL_POINT_SIZE_MAX, 8.0);
    GLfloat point_att[3] = {1, 0, 1e-4};
    glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, point_att);

    // other debugging stuff
    glLineWidth(3.0);

    // Register lighting/shadow CVars so they can be toggled via the console.
    // value=1 is already set in the definitions, so features work even before this.
    cv_grstaticlighting.Reg();
    cv_grshadows.Reg();
    cv_grcoronas.Reg();
    cv_grcoronasize.Reg();
    cv_monball_light.Reg();

    // Populate the data-driven light emitter table with hardcoded defaults.
    InitLightDefs();

    // Build the blob shadow texture.
    InitShadowTexture();
}

/// Clean up stuffage so we can start drawing a new frame.
void OGLRenderer::StartFrame()
{
    if (postfx.IsActive())
        postfx.BindSceneFBO();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ClearDrawColorAndLights();
}

/// Done with drawing. Swap buffers.
void OGLRenderer::FinishFrame()
{
    // Apply post-processing (bloom, SSAO) before swap.
    if (postfx.IsActive())
        postfx.ApplyEffects();
#ifdef SDL2
    SDL_GL_SwapWindow(screen); // Double buffered OpenGL goodness.
#else
    SDL_GL_SwapBuffers(); // Double buffered OpenGL goodness.
#endif
}

// Set default material colors and lights to bright white with full
// intensity.

void OGLRenderer::ClearDrawColorAndLights()
{
    GLfloat lmodel_ambient[] = {1.0, 1.0, 1.0, 1.0};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glColor4f(1.0, 1.0, 1.0, 1.0);
}

void OGLRenderer::SetGlobalColor(GLfloat *rgba)
{
}

// Writes a screen shot to the specified file. Writing is done in BMP
// format, as SDL has direct support of that. Adds proper file suffix
// when necessary. Returns true on success.
#ifndef SDL2
bool OGLRenderer::WriteScreenshot(const char *fname)
{
    int fnamelength;
    string finalname;
    SDL_Surface *buffer;
    bool success;

    if (screen == NULL)
        return false;

    /*
    if(screen->pixels == NULL) {
      CONS_Printf("Empty SDL surface. Can not take screenshot.\n");
      return false;
    }
    */

    if (fname == NULL)
        finalname = "DOOM000.bmp";
    else
    {
        fnamelength = strlen(fname);

        if (!strcmp(fname + fnamelength - 4, ".bmp") || !strcmp(fname + fnamelength - 4, ".BMP"))
            finalname = fname;
        else
        {
            finalname = fname;
            finalname += ".bmp";
        }
    }

    // Now we know the file name. Time to start the magic.

    // Potential endianness bug with masks?
    buffer = SDL_CreateRGBSurface(
        SDL_SWSURFACE, screen->w, screen->h, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
    if (!buffer)
    {
        CONS_Printf("Could not create SDL surface. SDL error: %s\n", SDL_GetError());
        return false;
    }

    SDL_LockSurface(buffer);
    glReadPixels(0, 0, screen->w, screen->h, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer->pixels);

    // OpenGL keeps the pixel data "upside down" for some reason. Flip
    // the surface.
    char *templine = new char[buffer->pitch];
    for (int j = 0; j < buffer->h / 2; j++)
    {
        char *p1;
        char *p2;
        p1 = static_cast<char *>(buffer->pixels) + j * buffer->pitch;
        p2 = static_cast<char *>(buffer->pixels) + (buffer->h - j - 1) * buffer->pitch;
        memcpy(templine, p1, buffer->pitch);
        memcpy(p1, p2, buffer->pitch);
        memcpy(p2, templine, buffer->pitch);
    }
    delete[] templine;

    SDL_UnlockSurface(buffer);
    success = !SDL_SaveBMP(buffer, finalname.c_str());
    SDL_FreeSurface(buffer);
    return success;
}
#else
// SDL2 screenshot not yet implemented
bool OGLRenderer::WriteScreenshot(const char *fname)
{
    (void)fname;
    CONS_Printf("Screenshot not yet implemented for SDL2\n");
    return false;
}
#endif

bool OGLRenderer::InitVideoMode(const int w, const int h, const int displaymode)
{
    Uint32 surfaceflags;
    int mindepth = 16;
    int temp;
    bool first_init;

    first_init = screen ? false : true;

    // Some platfroms silently destroy OpenGL textures when changing
    // resolution. Unload them all, just in case.
    materials.ClearGLTextures();

    // Destroy shader programs and GL resources while the old context is still current.
    postfx.Destroy();
    materials.ClearAllShaders();
    ClearFpShaders();
    DeletePBRNeutralTextures();
    if (normalmap_shaderprog)
    {
        delete normalmap_shaderprog;
        normalmap_shaderprog = NULL;
    }

    workinggl = false;

#ifdef SDL2
    // Clean up old context and window if they exist.
    // Context must be deleted BEFORE the window is destroyed.
    if (glCtx)
    {
        SDL_GL_DeleteContext(glCtx);
        glCtx = NULL;
    }
    if (screen)
    {
        SDL_DestroyWindow(screen);
        screen = NULL;
    }

    surfaceflags = SDL_WINDOW_OPENGL;
    if (displaymode == 1)
        surfaceflags |= SDL_WINDOW_FULLSCREEN;
    else if (displaymode == 2)
        surfaceflags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Color buffer depth from scr_depth (legacy_one behavior)
    {
        int depth = cv_scr_depth.value;
        if (depth <= 16)
        {
            SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   5);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  5);
            SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
            SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 16);
        }
        else if (depth <= 24)
        {
            SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
            SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
            SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 24);
        }
        else
        {
            SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
            SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
        }
    }

    // MSAA — must be set before window/context creation
    {
        int msaa = cv_msaa.value;
        if (msaa > 0)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
        }
        else
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        }
    }

    // In SDL2, we just try to create the window - no need for SDL_VideoModeOK
    screen = SDL_CreateWindow("Doom Legacy",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              w, h, surfaceflags);
    if (!screen && cv_msaa.value > 0)
    {
        // MSAA request rejected by driver — fall back to no anti-aliasing
        CONS_Printf(" MSAA %dx not available, falling back to no anti-aliasing.\n", cv_msaa.value);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        screen = SDL_CreateWindow("Doom Legacy",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  w, h, surfaceflags);
    }
    if (!screen)
    {
        CONS_Printf(" Could not obtain requested resolution.\n");
        return false;
    }

    // Create OpenGL context
    glCtx = SDL_GL_CreateContext(screen);
    if (!glCtx)
    {
        CONS_Printf(" Could not create OpenGL context.\n");
        return false;
    }
    SDL_GL_MakeCurrent(screen, glCtx);

    // VSync
    SDL_GL_SetSwapInterval(cv_vsync.value);

    // Enable MSAA if the driver provided multisampling
    if (cv_msaa.value > 0)
        glEnable(GL_MULTISAMPLE);

    // Initialize GLEW
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK)
    {
        CONS_Printf(" GLEW initialization failed: %s\n", glewGetErrorString(glewErr));
        return false;
    }

    int cbpp = 24; // Assume 24-bit for SDL2
#else
    surfaceflags = SDL_OPENGL;
    if (fullscreen)
        surfaceflags |= SDL_FULLSCREEN;

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Check that we get hicolor.
    int cbpp = SDL_VideoModeOK(w, h, 24, surfaceflags);
    if (cbpp < mindepth)
    {
        CONS_Printf(" Hicolor OpenGL mode not available.\n");
        return false;
    }

    screen = SDL_SetVideoMode(w, h, cbpp, surfaceflags);
    if (!screen)
    {
        CONS_Printf(" Could not obtain requested resolution.\n");
        return false;
    }
#endif

    // Initialize GLEW (if not already initialized in SDL2 path)
#ifndef SDL2
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK)
    {
        CONS_Printf(" GLEW initialization failed: %s\n", glewGetErrorString(glewErr));
        return false;
    }
#endif

    // This is the earlies possible point to print these since GL
    // context is not guaranteed to exist until the call to
    // SDL_SetVideoMode.
    if (first_init)
    {
        CONS_Printf(" OpenGL vendor:   %s\n", glGetString(GL_VENDOR));
        CONS_Printf(" OpenGL renderer: %s\n", glGetString(GL_RENDERER));
        const char *str = reinterpret_cast<const char *>(glGetString(GL_VERSION));
        glversion = str ? strtof(str, NULL) : 0;
        CONS_Printf(" OpenGL version:  %s\n", str);
    }

    CONS_Printf(" Color depth in bits: ");
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &temp);
    CONS_Printf("R%d ", temp);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &temp);
    CONS_Printf("G%d ", temp);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &temp);
    CONS_Printf("B%d.\n", temp);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &temp);
    CONS_Printf(" Alpha buffer depth %d bits.\n", temp);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &temp);
    CONS_Printf(" Depth buffer depth %d bits.\n", temp);

    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &temp);
    if (temp)
        CONS_Printf(" OpenGL mode is double buffered.\n");
    else
        CONS_Printf(" OpenGL mode is NOT double buffered.\n");

    // Print state and debug info.
    CONS_Printf(" Set OpenGL video mode %dx%dx%d", w, h, cbpp);
    CONS_Printf((displaymode == 1) ? " (fullscreen)\n" : (displaymode == 2) ? " (borderless)\n" : " (windowed)\n");

    // Calculate the screen's aspect ratio. Assumes square pixels.
#ifdef SDL2
    if (w == 1280 && h == 1024 && displaymode == 1)
        screenar = 4.0 / 3.0;
    else if (w == 320 && h == 200 && displaymode == 1)
        screenar = 4.0 / 3.0;
    else
        screenar = GLfloat(w) / h;
#else
    if (w == 1280 && h == 1024 && // Check a couple of exceptions.
        surfaceflags & SDL_FULLSCREEN)
        screenar = 4.0 / 3.0;
    else if (w == 320 && h == 200 && surfaceflags & SDL_FULLSCREEN)
        screenar = 4.0 / 3.0;
    else
        screenar = GLfloat(w) / h;
#endif

    CONS_Printf(" Screen aspect ratio %.2f.\n", screenar);
    CONS_Printf(" HUD aspect ratio %.2f.\n", hudar);

    workinggl = true;

    // Reset matrix transformations.
    SetFullScreenViewport();

    InitGLState();

    // Clear any old GL errors.
    while (glGetError() != GL_NO_ERROR)
        ;

    // Recompile shaders in the new GL context.
    InitNormalMapShader();
    CompileGLDEFSShaders();  // .fp shaders for GLDEFS materials (before AssignNormalMapShader)
    if (normalmap_shaderprog)
        materials.AssignNormalMapShader(normalmap_shaderprog);

    // Post-processing framework (bloom, SSAO).
    postfx.Destroy();
    postfx.Init(w, h);

    if (GLExtAvailable("GL_ARB_multitexture"))
        CONS_Printf(" GL multitexturing supported.\n");
    else
        CONS_Printf(" GL multitexturing not supported. Expect trouble.\n");

    if (GLExtAvailable("GL_ARB_texture_non_power_of_two"))
        CONS_Printf(" Non power of two textures supported.\n");
    else
        CONS_Printf(" Only power of two textures supported.\n");

    return true;
}

// Set up viewport projection matrix, and 2D mode using the new aspect ratio.
void OGLRenderer::SetFullScreenViewport()
{
#ifdef SDL2
    viewportw = SDL_GetWindowWidth(screen);
    viewporth = SDL_GetWindowHeight(screen);
#else
    viewportw = screen->w;
    viewporth = screen->h;
#endif
    viewportar = GLfloat(viewportw) / viewporth;
    glViewport(0, 0, viewportw, viewporth);
    Setup2DMode();
}

void OGLRenderer::SetViewport(unsigned vp)
{
    // Splitscreen.
    unsigned n = min(cv_splitscreen.value + 1, MAX_GLVIEWPORTS) - 1;

    if (vp > n)
        return;

    viewportdef_t *v = &gl_viewports[n][vp];

#ifdef SDL2
    viewportw = GLint(SDL_GetWindowWidth(screen) * v->w);
    viewporth = GLint(SDL_GetWindowHeight(screen) * v->h);
    viewportar = GLfloat(viewportw) / viewporth;
    glViewport(GLint(SDL_GetWindowWidth(screen) * v->x), GLint(SDL_GetWindowHeight(screen) * v->y), viewportw, viewporth);
#else
    viewportw = GLint(screen->w * v->w);
    viewporth = GLint(screen->h * v->h);
    viewportar = GLfloat(viewportw) / viewporth;
    glViewport(GLint(screen->w * v->x), GLint(screen->h * v->y), viewportw, viewporth);
#endif
}

/// Set up the GL matrices so that we can draw 2D stuff like menus.
void OGLRenderer::Setup2DMode()
{
    GLfloat extraoffx, extraoffy, extrascalex, extrascaley;
    consolemode = true;
    ClearDrawColorAndLights();

    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);

    if (viewportar > hudar)
    {
        extraoffx = (viewportar - hudar) / (viewportar * 2.0);
        extraoffy = 0.0;
        extrascalex = hudar / viewportar;
        extrascaley = 1.0;
    }
    else if (viewportar < hudar)
    {
        extraoffx = 0.0;
        extraoffy = (hudar - viewportar) / (hudar * 2.0);
        extrascalex = 1.0;
        extrascaley = viewportar / hudar;
    }
    else
    {
        extrascalex = extrascaley = 1.0;
        extraoffx = extraoffy = 0.0;
    }

    glTranslatef(extraoffx, extraoffy, 0.0);
    glScalef(extrascalex, extrascaley, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/// Setup GL matrices to render level graphics.
void OGLRenderer::Setup3DMode()
{
    consolemode = false;
    ClearDrawColorAndLights();

    glDisable(GL_LIGHTING);  // sector lighting is applied per-surface via glColor4f
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Read projection information from consvars. Currently only fov.
    fov = max(1.0f, min(GLfloat(cv_fov.value), 180.0f));

    // Load clipping planes from consvars.
    gluPerspective(fov * hudar / viewportar,
                   viewportar,
                   cv_grnearclippingplane.Get().Float(),
                   cv_grfarclippingplane.Get().Float());

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Now we can render items using their Doom coordinates directly.
}

// Draws a square defined by the parameters. The square is filled by
// the specified Material.
void OGLRenderer::Draw2DGraphic(GLfloat left,
                                GLfloat bottom,
                                GLfloat right,
                                GLfloat top,
                                Material *mat,
                                GLfloat texleft,
                                GLfloat texbottom,
                                GLfloat texright,
                                GLfloat textop)
{
    //  GLfloat scalex, scaley, offsetx, offsety;
    //  GLfloat texleft, texright, textop, texbottom;

    if (!workinggl)
        return;

    if (!consolemode)
        I_Error("Attempting to draw a 2D HUD graphic while in 3D mode.\n");

    //  printf("Drawing tex %d at (%.2f, %.2f) (%.2f, %.2f).\n", tex, top, left, bottom, right);

    mat->GLUse();
    glBegin(GL_QUADS);
    glTexCoord2f(texleft, texbottom);
    glVertex2f(left, bottom);
    glTexCoord2f(texright, texbottom);
    glVertex2f(right, bottom);
    glTexCoord2f(texright, textop);
    glVertex2f(right, top);
    glTexCoord2f(texleft, textop);
    glVertex2f(left, top);
    glEnd();
}

// Just like the earlier one, except the coordinates are given in Doom
// units. (screen is 320 wide and 200 high.)

void OGLRenderer::Draw2DGraphic_Doom(GLfloat x, GLfloat y, Material *mat, int flags)
{
    // OpenGL origin is at the bottom left corner. Doom origin is at top left.
    // Texture coordinates follow the Doom convention.

    /*
    if((x+width) > doomscreenw || (y+height) > doomscreenh)
      printf("Tex %d out of bounds: (%.2f, %.2f) (%.2f, %.2f).\n", tex, x, y, x+width, y+height);
    */
    GLfloat l, r, t, b;

    // location scaling
    if (flags & V_SLOC)
    {
        l = x / BASEVIDWIDTH;
        t = y / BASEVIDHEIGHT;
    }
    else
    {
#ifdef SDL2
        l = x / SDL_GetWindowWidth(screen);
        t = y / SDL_GetWindowHeight(screen);
#else
        l = x / screen->w;
        t = y / screen->h;
#endif
    }

    Material::TextureRef &tr = mat->tex[0];

    // size scaling
    // assume offsets in world units
    if (flags & V_SSIZE)
    {
        l -= mat->leftoffs / BASEVIDWIDTH;
        r = l + tr.worldwidth / BASEVIDWIDTH;

        t -= mat->topoffs / BASEVIDHEIGHT;
        b = t + tr.worldheight / BASEVIDHEIGHT;
    }
    else
    {
#ifdef SDL2
        l -= mat->leftoffs / SDL_GetWindowWidth(screen);
        r = l + tr.worldwidth / SDL_GetWindowWidth(screen);

        t -= mat->topoffs / SDL_GetWindowHeight(screen);
        b = t + tr.worldheight / SDL_GetWindowHeight(screen);
#else
        l -= mat->leftoffs / screen->w;
        r = l + tr.worldwidth / screen->w;

        t -= mat->topoffs / screen->h;
        b = t + tr.worldheight / screen->h;
#endif
    }

    Draw2DGraphic(l, 1 - b, r, 1 - t, mat);
}

/// Draw a semi-transparent console background.
/// Draws within the current Setup2DMode() coordinate space so that it is
/// aligned with text drawn via Material::Draw (both share the same
/// centering/aspect-ratio transform).
void OGLRenderer::DrawConsoleBackground(float height_frac, float alpha)
{
    if (!workinggl || !consolemode)
        return;

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Dark semi-transparent background (very dark blue-black, classic console look)
    glColor4f(0.0f, 0.0f, 0.05f, alpha);

    // GL Y: 0=bottom, 1=top in the Setup2DMode coordinate space.
    // Console covers the top height_frac of the display.
    float y_bottom = 1.0f - height_frac;
    glBegin(GL_QUADS);
    glVertex2f(0.0f, y_bottom);
    glVertex2f(1.0f, y_bottom);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(0.0f, 1.0f);
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}

void OGLRenderer::Draw2DGraphicFill_Doom(
    GLfloat x, GLfloat y, GLfloat width, GLfloat height, Material *mat)
{
    //  CONS_Printf("w: %f, h: %f, texw: %f, texh: %f.\n", width, height, texwidth, texheight);
    //  CONS_Printf("xrepeat %.2f, yrepeat %.2f.\n", width/texwidth, height/texheight);
    // here we may ignore offsets (original Doom behavior)
    Material::TextureRef &tr = mat->tex[0];

    Draw2DGraphic(x / BASEVIDWIDTH,
                  1 - (y + height) / BASEVIDHEIGHT,
                  (x + width) / BASEVIDWIDTH,
                  1 - y / BASEVIDHEIGHT,
                  mat,
                  0.0,
                  height / tr.worldheight,
                  width / tr.worldwidth,
                  0.0);
}

/// Currently a no-op. Possibly do something in the future.
void OGLRenderer::ClearAutomap()
{
}

/// Draws the specified map line to screen.
void OGLRenderer::DrawAutomapLine(const fline_t *line, const int color)
{
    if (!consolemode)
        I_Error("Trying to draw level map while in 3D mode.\n");

    // Set color.
    glColor3f(palette[color].r / 255.0, palette[color].g / 255.0, palette[color].b / 255.0);

    // Do not use a texture.
    glBindTexture(GL_TEXTURE_2D, 0);

    glBegin(GL_LINES);
    glVertex2f(line->a.x / GLfloat(BASEVIDWIDTH), 1.0 - line->a.y / GLfloat(BASEVIDHEIGHT));
    glVertex2f(line->b.x / GLfloat(BASEVIDWIDTH), 1.0 - line->b.y / GLfloat(BASEVIDHEIGHT));
    glEnd();
}

void OGLRenderer::RenderPlayerView(PlayerInfo *player)
{
    validcount++;

    // Set up the Map to be rendered. Needs to be done separately for each viewport, since the
    // engine can run several Maps at once.
    mp = player->mp;

    if (mp->glvertexes == NULL)
        I_Error("Trying to render level but level geometry is not set.\n");

    Setup3DMode();

    if (!mp->skybox_pov)
    {
        // This simple sky rendering algorithm uses 2D mode. We should
        // probably do something more fancy in the future.
        DrawSimpleSky();
    }
    else
    {
        // render skybox
        // TODO proper sequental rotation so we can do CTF_Face -style maps!
        phi = Degrees(player->pov->yaw + mp->skybox_pov->yaw);
        theta = Degrees(player->pov->pitch);

        Render3DView(mp->skybox_pov);
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glClear(GL_DEPTH_BUFFER_BIT);
        validcount++; // prepare to render same sectors again if necessary
    }

    // render main view
    phi = Degrees(player->pov->yaw);
    theta = Degrees(player->pov->pitch);
    Render3DView(player->pov);

    // render crosshair
    GLdouble chx, chy, chz; // Last one is a dummy.
    if (player->pawn)
    {
        GLfloat aimsine = 0.0;
        if (player->options.autoaim)
        {
            player->pawn->AimLineAttack(player->pawn->yaw, 3000, aimsine);
        }
        else
        {
            aimsine = Sin(player->pawn->pitch).Float();
        }
        player->pawn->LineTrace(player->pawn->yaw, 30000, aimsine, false);

        vec_t<fixed_t> target = trace.Point(trace.frac);

        // if (!(player->mp->maptic & 0xF))
        //   CONS_Printf("targ: %f, %f, %f, dist %f\n", target.x.Float(), target.y.Float(),
        //   target.z.Float(), trace.frac);

        GLdouble model[16];
        GLdouble proj[16];
        GLint vp[4];
        glGetDoublev(GL_MODELVIEW_MATRIX, model);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, vp);
        gluProject(target.x.Float(),
                   target.y.Float(),
                   target.z.Float(),
                   model,
                   proj,
                   vp,
                   &chx,
                   &chy,
                   &chz);

        /*
        // Draw a red dot there. Used for testing.
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glBindTexture(GL_TEXTURE_2D, 0);

        glColor3f(0.8, 0.0, 0.0);
        glBegin(GL_POINTS);
        glVertex3f(target.x.Float(), target.y.Float(), target.z.Float());
        glEnd();
        */
    }

    // Pretty soon we want to draw HUD graphics and stuff.
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    Setup2DMode();

    // Find the LocalPlayerInfo that owns this PlayerInfo so we use the correct
    // crosshair setting for each viewport rather than hardcoding LocalPlayers[0].
    LocalPlayerInfo *lpi = nullptr;
    for (int k = 0; k < NUM_LOCALPLAYERS; k++)
    {
        if (LocalPlayers[k].info == player)
        {
            lpi = &LocalPlayers[k];
            break;
        }
    }
    if (player->pawn && lpi && lpi->crosshair)
    {
        extern Material *crosshair[];
        int c = lpi->crosshair & 3;
        Material *mat = crosshair[c - 1];

        GLfloat top, left, bottom, right;

        left = chx / viewportw - mat->leftoffs / BASEVIDWIDTH;
        right = left + mat->worldwidth / BASEVIDWIDTH;
        top = chy / viewporth + mat->topoffs / BASEVIDHEIGHT;
        bottom = top - mat->worldheight / BASEVIDHEIGHT;

        Draw2DGraphic(left, bottom, right, top, mat);
    }

    // Draw weapon sprites.
    bool drawPsprites = (player->pov == player->pawn);

    if (drawPsprites && cv_psprites.value)
        DrawPSprites(player->pawn);
}

/// Set up state and draw a view of the level from the given viewpoint.
/// It is usually the place where the player is currently located.
/// NOTE: Leaves a modelview matrix on the stack, which _must_ be popped by the caller!
void OGLRenderer::Render3DView(Actor *pov)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    x = pov->pos.x.Float();
    y = pov->pos.y.Float();
    z = pov->GetViewZ().Float();

    // Set up camera to look through pov's eyes.
    glRotatef(-90.0 - theta, 1.0, 0.0, 0.0);
    glRotatef(90.0 - phi, 0.0, 0.0, 1.0);
    glTranslatef(-x, -y, -z);

    curssec = pov->subsector;

    // Collect dynamic lights for this frame (weapon flash + decorative emitters).
    CollectDynamicLights(pov);

    // Build frustum and we are ready to render.
    CalculateFrustum();
    RenderBSPNode(mp->numnodes - 1);

    // Draw all actors via thinker list. This is more reliable than
    // per-subsector sector-thinglist traversal, which misses actors in
    // sectors that the BSP renderer doesn't visit (e.g. live moving monsters).
    for (Thinker *th = mp->thinkercap.Next(); th != &mp->thinkercap; th = th->Next())
    {
        Actor *a = th->Inherits<Actor>();
        if (!a || !a->pres) continue;
        if (a->flags2 & MF2_DONTDRAW) continue;
        glDisable(GL_CULL_FACE);
        a->pres->Draw(a);
        glEnable(GL_CULL_FACE);
    }

    // Draw all blob shadows in one final pass after all floor geometry is in
    // the framebuffer.  The depth test clips each shadow quad to every floor
    // face it overlaps, so shadows cross subsector boundaries correctly.
    if (cv_grshadows.value)
        DrawAllBlobShadows();

    // Draw corona halos at dynamic light positions (additive glow over geometry).
    if (cv_grcoronas.value && cv_grdynamiclighting.value)
        DrawAllCoronas();
}

void OGLRenderer::DrawPSprites(PlayerPawn *p)
{
    // add all active psprites
    pspdef_t *psp = p->psprites;
    for (int i = 0; i < NUMPSPRITES; i++, psp++)
    {
        if (!psp->state)
            continue; // not active

        // decide which patch to use
        sprite_t *sprdef = sprites.Get(spritenames[psp->state->sprite]);
        if (!sprdef)
            continue; // sprite not loaded (missing WAD resource), skip gracefully
        int frame_idx = psp->state->frame & TFF_FRAMEMASK;
        if (frame_idx >= sprdef->numframes)
            continue; // frame out of range for this sprite, skip gracefully
        spriteframe_t *sprframe = &sprdef->spriteframes[frame_idx];

#ifdef PARANOIA
        if (!sprframe)
            I_Error("sprframes NULL for state %d\n", psp->state - weaponstates);
#endif

        Material *mat = sprframe->tex[0];

        // added:02-02-98:spriteoffset should be abs coords for psprites, based on 320x200
        GLfloat tx = psp->sx.Float(); // - t->leftoffs;
        GLfloat ty = psp->sy.Float(); // - t->topoffs;

        // lots of TODOs for psprites
        /*
        if (sprframe->flip[0])

        if (viewplayer->flags & MF_SHADOW)      // invisibility effect
        {
      if (viewplayer->powers[pw_invisibility] > 4*TICRATE
          || viewplayer->powers[pw_invisibility] & 8)
        }
        else if (fixedcolormap)
        else if (psp->state->frame & TFF_FULLBRIGHT)
        else
        // local light
        */
        // Apply psprite lighting: full-bright if fixed colormap (IR visor) or full-bright frame,
        // otherwise scale by sector light + extralight so weapon shading matches the viewport.
        bool fullbright = p->fixedcolormap || (psp->state->frame & TFF_FULLBRIGHT);
        if (fullbright)
        {
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
        else
        {
            float light = (p->subsector->sector->lightlevel + p->extralight * 16) / 255.0f;
            if (light > 1.0f) light = 1.0f;
            glColor4f(light, light, light, 1.0f);
        }
        Draw2DGraphic_Doom(tx, ty, mat, V_SCALE);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // restore default colour
    }
}

// Calculates the 2D view frustum of the current player view. Call
// before rendering BSP.

void OGLRenderer::CalculateFrustum()
{
    static const GLfloat fsize = 10000.0; // Depth of frustum, also a hack.

    // Build frustum points suitable for analysis.
    fr_cx = x;
    fr_cy = y;

    // Compute the horizontal half-FOV to match gluPerspective(fov*hudar/viewportar, viewportar).
    // gluPerspective takes a vertical FOV and an aspect ratio; the actual horizontal FOV is
    //   H_FOV = 2 * atan(tan(fovY/2) * aspect)
    // Using fovY = fov * hudar / viewportar and aspect = viewportar:
    float fovy_deg = fov * hudar / viewportar;
    float half_h_fov = (float)(atan(tan(fovy_deg * (M_PI / 360.0)) * viewportar) * (180.0 / M_PI));
    fr_lx = x + fsize * cos((phi + half_h_fov) * (M_PI / 180.0));
    fr_ly = y + fsize * sin((phi + half_h_fov) * (M_PI / 180.0));

    fr_rx = x + fsize * cos((phi - half_h_fov) * (M_PI / 180.0));
    fr_ry = y + fsize * sin((phi - half_h_fov) * (M_PI / 180.0));
}

// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
bool OGLRenderer::BBoxIntersectsFrustum(const bbox_t &bbox) const
{
    // If we have intersections, bbox area is visible.
    if (bbox.LineCrossesEdge(fr_cx, fr_cy, fr_lx, fr_ly))
        return true;
    if (bbox.LineCrossesEdge(fr_cx, fr_cy, fr_rx, fr_ry))
        return true;
    if (bbox.LineCrossesEdge(fr_lx, fr_ly, fr_rx, fr_ry)) // Is this really necessary?
        return true;

    // At this point the bbox is either entirely outside or entirely
    // inside the frustum. Find out which.
    vertex_t v1, v2;
    line_t l;
    l.v1 = &v1;
    l.v2 = &v2;
    v1.x = fr_cx;
    v1.y = fr_cy;
    v2.x = fr_lx;
    v2.y = fr_ly;
    l.dx = fr_lx - fr_cx;
    l.dy = fr_ly - fr_cy;

    if (P_PointOnLineSide(bbox.box[BOXLEFT], bbox.box[BOXTOP], &l))
        return false;

    v2.x = fr_rx;
    v2.y = fr_ry;
    l.dx = fr_rx - fr_cx;
    l.dy = fr_ry - fr_cy;

    if (!P_PointOnLineSide(bbox.box[BOXLEFT], bbox.box[BOXTOP], &l))
        return false;

    return true;
}

/// Walk through the BSP tree and render the level in back-to-front
/// order. Assumes that the OpenGL state is properly set elsewhere.
void OGLRenderer::RenderBSPNode(int nodenum)
{
    // Found a subsector (leaf node)?
    if (nodenum & NF_SUBSECTOR)
    {
        int nt = nodenum & ~NF_SUBSECTOR;
        if (curssec == NULL || CheckVis(curssec - mp->subsectors, nt))
            RenderGLSubsector(nt);
        return;
    }

    // otherwise keep traversing the tree
    // Use original nodes - they work fine for BSP traversal
    node_t *node = &mp->nodes[nodenum];

    // Decide which side the view point is on.
    int side = node->PointOnSide(x, y);

    // OpenGL requires back-to-front drawing for translucency effects to
    // work properly. Thus we first check the back.
    if (BBoxIntersectsFrustum(node->bbox[side ^ 1])) // sort of frustum culling
        RenderBSPNode(node->children[side ^ 1]);

    // Now we draw the front side.
    RenderBSPNode(node->children[side]);
}

/// Draws one single GL subsector polygon that is either a floor or a ceiling.
void OGLRenderer::RenderGlSsecPolygon(
    subsector_t *ss, GLfloat height, Material *mat, bool isFloor, GLfloat xoff, GLfloat yoff, int lightlevel)
{
    int loopstart, loopend, loopinc;

    int firstseg = ss->first_seg;
    int segcount = ss->num_segs;

    // GL subsector polygons are clockwise when viewed from above.
    // OpenGL polygons are defined counterclockwise. Thus we need to go
    // "backwards" when drawing the floor.
    if (isFloor)
    {
        loopstart = firstseg + segcount - 1;
        loopend = firstseg - 1;
        loopinc = -1;
    }
    else
    {
        loopstart = firstseg;
        loopend = firstseg + segcount;
        loopinc = 1;
    }

    glNormal3f(0.0, 0.0, -loopinc);
    mat->GLUse();

    // For normal-mapped flats, supply a world-space tangent along +X.
    // The floor normal is (0,0,±1), so T=(1,0,0) gives B=cross(N,T)=(0,±1,0),
    // correctly orienting the normal map red=+X, green=+Y on the floor plane.
    bool flatShaderActive = (mat->shader != NULL);
    if (flatShaderActive)
    {
        shader_attribs_t flat_sa;
        flat_sa.tangent[0] = 1.0f;
        flat_sa.tangent[1] = 0.0f;
        flat_sa.tangent[2] = 0.0f;
        mat->shader->SetAttributes(&flat_sa);
    }

    // Collect vertices first to build a triangle fan
    // For a convex subsector, this should work better than GL_POLYGON
    GLfloat vertices[512 * 3]; // Max 512 vertices
    GLfloat texcoords[512 * 2];
    int numverts = 0;

    // Collect vertices - always use v1 for all segs (like GZDoom does)
    // Using v1 for floor caused issues - always use consistent vertex
    for (int curseg = loopstart; curseg != loopend; curseg += loopinc)
    {
        // Use glsegs since we're rendering glsubsectors
        seg_t *seg = &mp->glsegs[curseg];
        GLfloat x, y, tx, ty;
        vertex_t *v = seg->v1;  // Always use v1 for consistency
        x = v->x.Float();
        y = v->y.Float();

        tx = (x + xoff) / mat->worldwidth;
        ty = 1.0 - (y - yoff) / mat->worldheight;

        vertices[numverts * 3 + 0] = x;
        vertices[numverts * 3 + 1] = y;
        vertices[numverts * 3 + 2] = height;
        texcoords[numverts * 2 + 0] = tx;
        texcoords[numverts * 2 + 1] = ty;
        numverts++;

        //    printf("(%.2f, %.2f)\n", x, y);
    }

    // Base static light for this sector (same for every vertex).
    float base = cv_grstaticlighting.value ? LightLevelToFloat(lightlevel) : 1.0f;
    bool dynActive = cv_grdynamiclighting.value && !framelights.empty();
    // Shaded flat: upload dynamic lights as uniforms, skip CPU per-vertex accum.
    if (flatShaderActive && dynActive)
        SetShaderDynamicLights(mat->shader);

    // Sector colormap tint (Boom/Hexen per-sector colored lighting).
    // The rgba field is packed as R+(G<<8)+(B<<16)+(alpha<<24) where alpha
    // is 0-25.  We lerp from white (no tint) toward the tint color.
    float cmr = 1.0f, cmg = 1.0f, cmb = 1.0f;
    {
        fadetable_t *ec = ss->sector->extra_colormap;
        if (ec && ec->rgba != 0)
        {
            float tr = ((ec->rgba      ) & 0xff) / 255.0f;
            float tg = ((ec->rgba >> 8 ) & 0xff) / 255.0f;
            float tb = ((ec->rgba >> 16) & 0xff) / 255.0f;
            float ta = ((ec->rgba >> 24) & 0xff) / 25.0f;
            if (ta > 1.0f) ta = 1.0f;
            cmr = 1.0f + ta * (tr - 1.0f);
            cmg = 1.0f + ta * (tg - 1.0f);
            cmb = 1.0f + ta * (tb - 1.0f);
        }
    }

    // Draw as triangle fan or wireframe for debugging
    if (r_wireframe)
    {
        glDisable(GL_TEXTURE_2D);
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f); // Yellow wireframe
        glBegin(GL_LINES);
        for (int i = 0; i < numverts; i++)
        {
            glVertex3f(vertices[0], vertices[1], vertices[2]);
            glVertex3f(vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2]);
            if (i < numverts - 1)
            {
                glVertex3f(vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2]);
                glVertex3f(vertices[(i + 1) * 3 + 0], vertices[(i + 1) * 3 + 1], vertices[(i + 1) * 3 + 2]);
            }
        }
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        // Per-vertex lighting: GL_SMOOTH interpolates between vertices so dynamic
        // lights fade smoothly across face boundaries instead of snapping.
        glBegin(GL_POLYGON);
        for (int i = 0; i < numverts; i++)
        {
            float vx = vertices[i * 3 + 0];
            float vy = vertices[i * 3 + 1];
            float fr = base * cmr, fg = base * cmg, fb = base * cmb;
            if (dynActive && !flatShaderActive)
                AccumDynLight(vx, vy, height, fr, fg, fb);
            if (fr > 1.0f) fr = 1.0f;
            if (fg > 1.0f) fg = 1.0f;
            if (fb > 1.0f) fb = 1.0f;
            glColor4f(fr, fg, fb, 1.0f);
            glTexCoord2f(texcoords[i * 2 + 0], texcoords[i * 2 + 1]);
            glVertex3f(vx, vy, vertices[i * 3 + 2]);
        }
        glEnd();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

#if 0
  {
    // draw subsector boundaries (for debugging)
    glDisable(GL_TEXTURE_2D);

    float c = 0.2+fabs(cos(mp->maptic / 35.0f));
    glColor4f(0.7, 0.7, 1.0, c);
    glBegin(GL_LINE_LOOP);
    for (int curseg = loopstart; curseg != loopend; curseg += loopinc)
      {
	vertex_t *v = mp->segs[curseg].v1;
	glVertex3f(v->x.Float(), v->y.Float(), height);
      }
    glEnd();

    glColor3f(0.8, 0.0, 0.0);
    glBegin(GL_POINTS);
    for (int curseg = loopstart; curseg != loopend; curseg += loopinc)
      {
	vertex_t *v = mp->segs[curseg].v1;
	glVertex3f(v->x.Float(), v->y.Float(), height);
      }
    glEnd();

    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0, 1.0, 1.0, 1.0);
  }
#endif
}

// Draw the floor and ceiling of a single GL subsector. The rendering
// order is flats->wall segs->items, so there is no occlusion. In the
// future render also 3D floors and polyobjs.
void OGLRenderer::RenderGLSubsector(int num)
{
    int curseg;

    ffloor_t *ff;
    Material *fmat;
    if (num < 0 || num >= mp->numglsubsectors)
        return;

    // Use GL subsectors for floor/ceiling
    subsector_t *ss = &mp->glsubsectors[num];
    int firstseg = ss->first_seg;
    int segcount = ss->num_segs;
    sector_t *s = ss->sector;

    // Skip degenerate subsectors
    if (!s || segcount <= 0)
        return;

    // Distance fog: darker sectors have denser fog (shorter visible range).
    // Phase F: when the sector has a fade_color set via extra_colormap,
    // use it as the fog end color (Doom 64-style colored fade).
    if (cv_grstaticlighting.value)
    {
        float light = (float)s->lightlevel / 255.0f;
        // Default fog color is black; use extra_colormap fade tint when present.
        GLfloat fogColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        {
            fadetable_t *ec = s->extra_colormap;
            if (ec && ec->rgba != 0)
            {
                float ta = ((ec->rgba >> 24) & 0xff) / 25.0f;
                if (ta > 0.3f)  // only apply fog tint if tint alpha is significant
                {
                    fogColor[0] = ((ec->rgba      ) & 0xff) / 255.0f;
                    fogColor[1] = ((ec->rgba >> 8 ) & 0xff) / 255.0f;
                    fogColor[2] = ((ec->rgba >> 16) & 0xff) / 255.0f;
                }
            }
        }
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, fogColor);
        float fogStart = light * 1600.0f;
        float fogEnd   = fogStart + 800.0f;
        glFogf(GL_FOG_START, fogStart);
        glFogf(GL_FOG_END,   fogEnd);
    }
    else
    {
        glDisable(GL_FOG);
    }

    // Draw ceiling texture, skip sky flats.
    fmat = s->ceilingpic;
    if (fmat && !s->SkyCeiling())
        RenderGlSsecPolygon(ss,
                            s->ceilingheight.Float(),
                            fmat,
                            false,
                            s->ceiling_xoffs.Float(),
                            s->ceiling_yoffs.Float(),
                            s->lightlevel);

    // Then the floor.
    fmat = s->floorpic;
    if (fmat && !s->SkyFloor())
        RenderGlSsecPolygon(
            ss, s->floorheight.Float(), fmat, true, s->floor_xoffs.Float(), s->floor_yoffs.Float(), s->lightlevel);

    // Draw the walls of this subsector.
    for (curseg = firstseg; curseg < firstseg + segcount; curseg++)
        RenderGLSeg(curseg);

    // A quick hack: draw 3D floors here. To do it Right, they (and
    // things in this subsector) need to be depth-sorted.
    for (ff = s->ffloors; ff; ff = ff->next)
    {

        GLfloat textop, texbottom, texleft, texright;
        fmat = *ff->toppic;
        RenderGlSsecPolygon(ss, ff->topheight->Float(), fmat, true, 0, 0, s->lightlevel);
        fmat = *ff->bottompic;
        RenderGlSsecPolygon(ss, ff->bottomheight->Float(), fmat, false, 0, 0, s->lightlevel);

        // Draw "edges" of 3D floor.

        // Sanity check.
        line_t *mline = ff->master;
        if (mline == NULL)
            continue;
        side_t *mside = mline->sideptr[0];
        if (mside == NULL)
            continue;
        fmat = mside->midtexture;
        if (fmat == NULL)
            continue;

        // Calculate texture offsets. (They are the same for every edge.)
        GLfloat fsheight =
            mside->sector->ceilingheight.Float() - mside->sector->floorheight.Float();
        if (mline->flags & ML_DONTPEGBOTTOM)
        {
            texbottom = mside->rowoffset.Float();
            textop = texbottom - fsheight;
        }
        else
        {
            textop = mside->rowoffset.Float();
            texbottom = textop + fsheight;
        }
        texleft = mside->textureoffset.Float();

        for (curseg = firstseg; curseg < firstseg + segcount; curseg++)
        {

            fixed_t nx, ny;
            seg_t *s = &(mp->segs[curseg]);
            texright = texleft + s->length;
            vertex_t *v1 = s->v1;
            vertex_t *v2 = s->v2;

            // Surface normal points to the opposite direction than if we
            // were drawing a wall, so swap endpoints (and texcoords!)
            DrawSingleQuad(fmat,
                           v2,
                           v1,
                           mside->sector->floorheight.Float(),
                           mside->sector->ceilingheight.Float(),
                           texright / fmat->worldwidth,
                           texleft / fmat->worldwidth,
                           textop / fmat->worldheight,
                           texbottom / fmat->worldheight);
        }
    }

}

/// Calculates the necessary numbers to render upper, middle, and
/// lower wall segments. Any quad whose Material pointer is not NULL
/// after this function is to be rendered.

void OGLRenderer::GetSegQuads(int num, quad &u, quad &m, quad &l) const
{
    side_t *lsd;  // Local sidedef.
    side_t *rsd;  // Remote sidedef.
    sector_t *ls; // Local sector.
    sector_t *rs; // Remote sector.

    GLfloat rs_floor, rs_ceil, ls_floor, ls_ceil; // Floor and ceiling heights.
    GLfloat textop, texbottom, texleft, texright;
    GLfloat utexhei, ltexhei;
    GLfloat ls_height, tex_yoff;
    Material *uppertex, *middletex, *lowertex;

    u.m = m.m = l.m = NULL;

    if (num < 0 || num >= mp->numsegs)
        return;

    seg_t *s = &(mp->segs[num]);
    line_t *ld = s->linedef;

    // Minisegs don't have wall textures so no point in drawing them.
    if (ld == NULL)
        return;

    if (ld->v1->x.Float() == 704 && ld->v1->y.Float() == -3552)
        utexhei = 0;

    // Mark this linedef as visited so it gets drawn on the automap.
    ld->flags |= ML_MAPPED;

    u.v1 = m.v1 = l.v1 = s->v1;
    u.v2 = m.v2 = l.v2 = s->v2;
    // Default: no colormap tint
    u.cmr = u.cmg = u.cmb = 1.0f;
    m.cmr = m.cmg = m.cmb = 1.0f;
    l.cmr = l.cmg = l.cmb = 1.0f;

    if (s->side == 0)
    {
        lsd = ld->sideptr[0];
        rsd = ld->sideptr[1];
    }
    else
    {
        lsd = ld->sideptr[1];
        rsd = ld->sideptr[0];
    }

    // This should never happen, but we are paranoid.
    if (lsd == NULL)
        return;

    ls = lsd->sector;
    ls_floor = ls->floorheight.Float();
    ls_ceil = ls->ceilingheight.Float();
    ls_height = ls_ceil - ls_floor;

    // Compute colormap tint from front sector's extra_colormap.
    {
        fadetable_t *ec = ls->extra_colormap;
        if (ec && ec->rgba != 0)
        {
            float tr = ((ec->rgba      ) & 0xff) / 255.0f;
            float tg = ((ec->rgba >> 8 ) & 0xff) / 255.0f;
            float tb = ((ec->rgba >> 16) & 0xff) / 255.0f;
            float ta = ((ec->rgba >> 24) & 0xff) / 25.0f;
            if (ta > 1.0f) ta = 1.0f;
            u.cmr = m.cmr = l.cmr = 1.0f + ta * (tr - 1.0f);
            u.cmg = m.cmg = l.cmg = 1.0f + ta * (tg - 1.0f);
            u.cmb = m.cmb = l.cmb = 1.0f + ta * (tb - 1.0f);
        }
    }

    // Store front sector lightlevel for wall lighting
    int ls_lightlevel = ls->lightlevel;

    // Fake contrast: N/S-facing walls are brighter, E/W-facing walls are darker.
    // Matches Doom's software renderer which uses line slope to vary brightness.
    {
        float dx = fabsf((s->v2->x - s->v1->x).Float());
        float dy = fabsf((s->v2->y - s->v1->y).Float());
        int lightdelta = (dy > dx) ? +18 : -18;
        int adjusted = ls_lightlevel + lightdelta;
        ls_lightlevel = adjusted < 0 ? 0 : adjusted > 255 ? 255 : adjusted;
    }

    if (rsd)
    {
        rs = rsd->sector;
        rs_floor = rs->floorheight.Float();
        rs_ceil = rs->ceilingheight.Float();
        utexhei = ls_ceil - rs_ceil;
        ltexhei = rs_floor - ls_floor;
    }
    else
    {
        rs = NULL;
        rs_floor = 0.0;
        rs_ceil = 0.0;
    }

    tex_yoff = lsd->rowoffset.Float();
    uppertex = lsd->toptexture;
    middletex = lsd->midtexture;
    lowertex = lsd->bottomtexture;

    // Texture X offsets.
    texleft = lsd->textureoffset.Float() + s->offset.Float();
    texright = texleft + s->length;

    // Ready to draw textures.
    if (rs != NULL)
    {
        // Upper texture.
        if (rs_ceil < ls_ceil && uppertex)
        {
            if (ld->flags & ML_DONTPEGTOP)
            {
                textop = tex_yoff;
                texbottom = textop + utexhei;
            }
            else
            {
                texbottom = tex_yoff;
                textop = texbottom - utexhei;
            }

            // If the front and back ceilings are sky, do not draw this
            // upper texture.
            if (!ls->SkyCeiling() || !rs->SkyCeiling())
            {
                u.m = uppertex;
                u.bottom = rs_ceil;
                u.top = ls_ceil;
                u.t.l = texleft / uppertex->worldwidth;
                u.t.r = texright / uppertex->worldwidth;
                u.t.t = textop / uppertex->worldheight;
                u.t.b = texbottom / uppertex->worldheight;
                u.lightlevel = ls_lightlevel;
            }
        }

        if (rs_floor > ls_floor && lowertex)
        {
            // Lower textures are a major PITA.
            if (ld->flags & ML_DONTPEGBOTTOM)
            {
                texbottom = tex_yoff + ls_height - lowertex->worldheight;
                textop = texbottom - ltexhei;
            }
            else
            {
                textop = tex_yoff;
                texbottom = textop + ltexhei;
            }
            l.m = lowertex;
            l.bottom = ls_floor;
            l.top = rs_floor;
            l.t.l = texleft / lowertex->worldwidth;
            l.t.r = texright / lowertex->worldwidth;
            l.t.t = textop / lowertex->worldheight;
            l.t.b = texbottom / lowertex->worldheight;
            l.lightlevel = ls_lightlevel;
        }

        // Double sided middle textures do not repeat, so we need some
        // trickery.
        if (middletex != 0 && ls_floor < rs_ceil && ls_ceil > rs_floor)
        {
            GLfloat top, bottom;
            if (ld->flags & ML_DONTPEGBOTTOM)
            {
                if (ls_floor < rs_floor)
                    bottom = rs_floor;
                else
                    bottom = ls_floor;
                top = bottom + middletex->worldheight;
                // Clamp to the lower ceiling so the texture doesn't bleed above
                // the adjacent sector's ceiling.
                GLfloat min_ceil = (ls_ceil < rs_ceil) ? ls_ceil : rs_ceil;
                if (top > min_ceil)
                    top = min_ceil;
            }
            else
            {
                if (ls_ceil < rs_ceil)
                    top = ls_ceil;
                else
                    top = rs_ceil;
                bottom = top - middletex->worldheight;
            }
            m.m = middletex;
            m.bottom = bottom;
            m.top = top;
            m.t.l = texleft / middletex->worldwidth;
            m.t.r = texright / middletex->worldwidth;
            m.t.t = 0.0;
            m.t.b = 1.0;
            m.lightlevel = ls_lightlevel;
        }
    }
    else if (middletex)
    {
        // Single sided middle texture.
        if (ld->flags & ML_DONTPEGBOTTOM)
        {
            texbottom = tex_yoff;
            textop = texbottom - ls_height;
        }
        else
        {
            textop = tex_yoff;
            texbottom = textop + ls_height;
        }
        m.m = middletex;
        m.bottom = ls_floor;
        m.top = ls_ceil;
        m.t.l = texleft / middletex->worldwidth;
        m.t.r = texright / middletex->worldwidth;
        m.t.t = textop / middletex->worldheight;
        m.t.b = texbottom / middletex->worldheight;
        m.lightlevel = ls_lightlevel;
    }
}

/// Renders one single GL seg. Minisegs and invalid parameters are
/// silently ignored.
void OGLRenderer::RenderGLSeg(int num)
{

    quad u, m, l;

    GetSegQuads(num, u, m, l);

    // For now, just render the quads. In the future, split each quad
    // along 3D floors and change light levels accordingly.
    if (u.m)
        DrawSingleQuad(&u);
    if (m.m)
        DrawSingleQuad(&m);
    if (l.m)
        DrawSingleQuad(&l);
}

void OGLRenderer::DrawSingleQuad(const quad *q) const
{
    DrawSingleQuad(q->m, q->v1, q->v2, q->bottom, q->top, q->t.l, q->t.r, q->t.t, q->t.b, q->lightlevel,
                   q->cmr, q->cmg, q->cmb);
}

/// Draw a single textured wall segment.
void OGLRenderer::DrawSingleQuad(Material *m,
                                 vertex_t *v1,
                                 vertex_t *v2,
                                 GLfloat lower,
                                 GLfloat upper,
                                 GLfloat texleft,
                                 GLfloat texright,
                                 GLfloat textop,
                                 GLfloat texbottom,
                                 int lightlevel,
                                 float cmr,
                                 float cmg,
                                 float cmb) const
{
    m->GLUse();

    shader_attribs_t sa; // TEST
    sa.tangent[0] = (v2->x - v1->x).Float();
    sa.tangent[1] = (v2->y - v1->y).Float();
    sa.tangent[2] = 0;

    glNormal3f(sa.tangent[1], -sa.tangent[0], 0.0);

    if (m->shader)
        m->shader->SetAttributes(&sa);

    // Per-vertex lighting so dynamic lights blend smoothly at face boundaries.
    float base = cv_grstaticlighting.value ? LightLevelToFloat(lightlevel) : 1.0f;
    bool dynActive = cv_grdynamiclighting.value && !framelights.empty();
    // When a GLSL shader is active the dynamic lights are passed as uniforms
    // (per-pixel, normal-map aware).  Skip CPU vertex-color accumulation for
    // those surfaces so lights are not double-counted.
    bool shaderActive = (m->shader != NULL);
    if (shaderActive && dynActive)
        SetShaderDynamicLights(m->shader);
    float x1 = v1->x.Float(), y1 = v1->y.Float();
    float x2 = v2->x.Float(), y2 = v2->y.Float();

    // Helper: compute clamped color at a world point and call glColor4f.
    // cmr/cmg/cmb are sector colormap tint multipliers (1.0 = no tint).
    // Written inline to avoid a lambda (C++11 lambdas can't call non-const member fns easily here).
#define VERT_COLOR(vx, vy, vz) \
    { float fr = base * cmr, fg = base * cmg, fb = base * cmb; \
      if (dynActive && !shaderActive) AccumDynLight(vx, vy, vz, fr, fg, fb); \
      if (fr > 1.0f) fr = 1.0f; if (fg > 1.0f) fg = 1.0f; if (fb > 1.0f) fb = 1.0f; \
      glColor4f(fr, fg, fb, 1.0f); }

    glBegin(GL_QUADS);
    VERT_COLOR(x1, y1, lower); glTexCoord2f(texleft,  texbottom); glVertex3f(x1, y1, lower);
    VERT_COLOR(x2, y2, lower); glTexCoord2f(texright, texbottom); glVertex3f(x2, y2, lower);
    VERT_COLOR(x2, y2, upper); glTexCoord2f(texright, textop);    glVertex3f(x2, y2, upper);
    VERT_COLOR(x1, y1, upper); glTexCoord2f(texleft,  textop);    glVertex3f(x1, y1, upper);
    glEnd();

#undef VERT_COLOR
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// Draws the specified item (weapon, monster, flying rocket etc) in
// the 3D view. Translations and rotations are done with OpenGL
// matrices.

void OGLRenderer::DrawSpriteItem(const vec_t<fixed_t> &pos, Material *mat, int flags, GLfloat alpha)
{
    // You can't draw the invisible.
    if (!mat)
        return;

    if (alpha < 1.0)
    {
        glColor4f(1.0, 1.0, 1.0, alpha); // set material params
        // glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // on by default
    }

    GLfloat top, bottom, left, right;
    GLfloat texleft, texright, textop, texbottom;

    if (flags & FLIP_X)
    {
        texleft = 1.0;
        texright = 0.0;
    }
    else
    {
        texleft = 0.0;
        texright = 1.0;
    }

    // Shouldn't these be the other way around?
    texbottom = 1.0;
    textop = 0.0;

    left = mat->leftoffs;
    right = left - mat->worldwidth;

    top = mat->topoffs;
    bottom = top - mat->worldheight;

    // If the sprite extends below the actor's origin (floor level), lift it up
    // so its bottom edge sits exactly on the floor instead of clipping through it.
    float zoffset = (bottom < 0) ? -bottom : 0.0f;
    top += zoffset;
    bottom = 0.0f;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glTranslatef(pos.x.Float(), pos.y.Float(), pos.z.Float());
    glRotatef(phi, 0.0, 0.0, 1.0);
    glNormal3f(-1.0, 0.0, 0.0);

    mat->GLUse();

    // Sky textures must not tile — clamp to avoid seam artefacts at edges.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBegin(GL_QUADS);
    glTexCoord2f(texleft, texbottom);
    glVertex3f(0.0, left, bottom);
    glTexCoord2f(texright, texbottom);
    glVertex3f(0.0, right, bottom);
    glTexCoord2f(texright, textop);
    glVertex3f(0.0, right, top);
    glTexCoord2f(texleft, textop);
    glVertex3f(0.0, left, top);
    glEnd();

    /* The world famous black triangles look.
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(0.0, 0.0, 0.0, 1.0);
    glBegin(GL_TRIANGLES);
    glVertex3f(0.0, 0.0, 128.0);
    glVertex3f(0.0, 32.0, 0.0);
    glVertex3f(0.0, -32.0, 0.0);
    glEnd();
    */

    // Leave the matrix stack as it was.
    glPopMatrix();

    glColor4f(1.0, 1.0, 1.0, 1.0); // back to normal material params
}

/// Check for visibility between the given glsubsectors. Returns true
/// if you can see from one to the other and false if not.
bool OGLRenderer::CheckVis(int fromss, int toss)
{
    byte *vis;
    if (mp->glvis == NULL)
        return true;

    vis = mp->glvis + (((mp->numsubsectors + 7) / 8) * fromss);
    if (vis[toss >> 3] & (1 << (toss & 7)))
        return true;
    return false;
}

/// Draws the background sky texture. Very simple, ignores looking up/down.
void OGLRenderer::DrawSimpleSky()
{
    GLfloat left, right, top, bottom, texleft, texright, textop, texbottom;

    GLboolean isdepth = 0;
    glGetBooleanv(GL_DEPTH_TEST, &isdepth);
    if (isdepth)
        glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    textop = 0.0; // And yet again these should be the other way around.
    texbottom = 1.0;
    top = 1.0;
    bottom = 0.0;
    left = 0.0;
    right = 1.0;

    GLfloat fovx = fov * viewportar;
    fovx *= 2.0; // I just love magic numbers. Don't you?

    texleft = (2.0 * phi + fovx) / 720.0;
    texright = (2.0 * phi - fovx) / 720.0;

    mp->skytexture->GLUse();
    glBegin(GL_QUADS);
    glTexCoord2f(texleft, texbottom);
    glVertex2f(left, bottom);
    glTexCoord2f(texright, texbottom);
    glVertex2f(right, bottom);
    glTexCoord2f(texright, textop);
    glVertex2f(right, top);
    glTexCoord2f(texleft, textop);
    glVertex2f(left, top);
    glEnd();

    if (isdepth)
        glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}
