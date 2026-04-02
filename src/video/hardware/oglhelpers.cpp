// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: oglhelpers.cpp 515 2007-12-22 14:51:46Z jussip $
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
/// Utility functions for OpenGL renderer.

#include "hardware/oglhelpers.hpp"
#include "command.h"
#include "cvars.h"
#include "doomdef.h"
#include "i_video.h"
#include "m_bbox.h"
#include "p_maputl.h"
#include "r_data.h"
#include "r_defs.h"

static const GLubyte *gl_extensions = NULL;

static byte lightleveltonumlut[256];

// This array has the fractional screen coordinates necessary to render multiple player
// views onto the screen. The first index tells how many total viewports-1
// there are on this screen. The second one tells the viewport number
// and must be less or equal to the first one.
viewportdef_t gl_viewports[MAX_GLVIEWPORTS][MAX_GLVIEWPORTS] = {
    // 1 Player
    {{0.0, 0.0, 1.0, 1.0}},

    // 2 players
    {{0.0, 0.5, 1.0, 0.5}, {0.0, 0.0, 1.0, 0.5}},

    // 3 players
    {{0.0, 0.5, 1.0, 0.5}, {0.0, 0.0, 0.5, 0.5}, {0.5, 0.0, 0.5, 0.5}},

    // 4 players
    {{0.0, 0.5, 0.5, 0.5}, {0.5, 0.5, 0.5, 0.5}, {0.0, 0.0, 0.5, 0.5}, {0.5, 0.0, 0.5, 0.5}}};

// Only list mipmapping filter modes, since we always use them.
CV_PossibleValue_t grfiltermode_cons_t[] = {{0, "NN"}, {1, "LN"}, {2, "NL"}, {3, "LL"}, {0, NULL}};
consvar_t cv_grfiltermode = {"gr_filtermode", "NN", CV_SAVE, grfiltermode_cons_t, NULL};

CV_PossibleValue_t granisotropy_cons_t[] = {{1, "MIN"}, {16, "MAX"}, {0, NULL}};
consvar_t cv_granisotropy = {"gr_anisotropy", "1", CV_SAVE, granisotropy_cons_t, NULL};

consvar_t cv_grnearclippingplane = {"gr_nearclippingplane", "0.9", CV_SAVE | CV_FLOAT, NULL};
consvar_t cv_grfarclippingplane = {"gr_farclippingplane", "16384.0", CV_SAVE | CV_FLOAT, NULL};

consvar_t cv_grdynamiclighting = {"gr_dynamiclighting", "On", CV_SAVE, CV_OnOff};
consvar_t cv_grcoronas = {"gr_coronas", "On", CV_SAVE, CV_OnOff};
consvar_t cv_grcoronasize = {"gr_coronasize", "1", CV_SAVE | CV_FLOAT, NULL};

static void CV_Gamma_OnChange();
static CV_PossibleValue_t gamma_cons_t[] = {{1, "MIN"}, {20, "MAX"}, {0, NULL}};
consvar_t cv_grgammared = {"gr_gammared", "10", CV_SAVE | CV_CALL, gamma_cons_t, CV_Gamma_OnChange};
consvar_t cv_grgammagreen = {
    "gr_gammagreen", "10", CV_SAVE | CV_CALL, gamma_cons_t, CV_Gamma_OnChange};
consvar_t cv_grgammablue = {
    "gr_gammablue", "10", CV_SAVE | CV_CALL, gamma_cons_t, CV_Gamma_OnChange};

static void CV_Gamma_OnChange()
{
    I_SetGamma(
        cv_grgammared.value / 10.0f, cv_grgammagreen.value / 10.0f, cv_grgammablue.value / 10.0f);
}

consvar_t cv_grfog = {"gr_fog", "On", CV_SAVE, CV_OnOff};
consvar_t cv_grfogcolor = {"gr_fogcolor", "000000", CV_SAVE, NULL};
consvar_t cv_grfogdensity = {"gr_fogdensity", "100", CV_SAVE, CV_Unsigned};

static CV_PossibleValue_t normalmapstrength_cons_t[] = {{0, "MIN"}, {20, "MAX"}, {0, NULL}};
consvar_t cv_grnormalmapstrength = {"gr_normalmapstrength", "10", CV_SAVE, normalmapstrength_cons_t};

// Quality preset CVar — bundles several settings into one choice
static void CV_GLQuality_OnChange()
{
    switch (cv_glquality.value)
    {
        case 0: // Low
            cv_granisotropy.Set(1);
            cv_grfiltermode.Set(0);
            cv_grshadows.Set(0);
            cv_grdynamiclighting.Set(0);
            cv_grcoronas.Set(0);
            cv_grfog.Set(0);
            break;
        case 1: // Medium
            cv_granisotropy.Set(4);
            cv_grfiltermode.Set(2);
            cv_grshadows.Set(1);
            cv_grdynamiclighting.Set(0);
            cv_grcoronas.Set(1);
            cv_grfog.Set(1);
            break;
        case 2: // High
            cv_granisotropy.Set(8);
            cv_grfiltermode.Set(3);
            cv_grshadows.Set(1);
            cv_grdynamiclighting.Set(1);
            cv_grcoronas.Set(1);
            cv_grfog.Set(1);
            break;
        case 3: // Ultra
            cv_granisotropy.Set(16);
            cv_grfiltermode.Set(3);
            cv_grshadows.Set(1);
            cv_grdynamiclighting.Set(1);
            cv_grcoronas.Set(1);
            cv_grfog.Set(1);
            break;
        default:
            break;
    }
}

static CV_PossibleValue_t glquality_cons_t[] = {
    {0,"Low"}, {1,"Medium"}, {2,"High"}, {3,"Ultra"}, {0,NULL}
};
consvar_t cv_glquality = {
    "gr_quality", "Medium", CV_SAVE | CV_CALL, glquality_cons_t, CV_GLQuality_OnChange
};

// Post-processing CVars
consvar_t cv_grbloom           = {"gr_bloom",           "Off", CV_SAVE, CV_OnOff, NULL, 0};
consvar_t cv_grbloomthreshold  = {"gr_bloomthreshold",  "0.8", CV_SAVE | CV_FLOAT, NULL, NULL, 0};
consvar_t cv_grbloomstrength   = {"gr_bloomstrength",   "0.4", CV_SAVE | CV_FLOAT, NULL, NULL, 0};
consvar_t cv_grssao            = {"gr_ssao",            "Off", CV_SAVE, CV_OnOff, NULL, 0};
consvar_t cv_grssaostrength    = {"gr_ssaostrength",    "0.6", CV_SAVE | CV_FLOAT, NULL, NULL, 0};
consvar_t cv_grdeferred        = {"gr_deferred",        "Off", CV_SAVE, CV_OnOff, NULL, 0};

// Number of deferred lights that receive cube shadow maps (Phase 3.2). 0 = off, 1-4.
CV_PossibleValue_t cubeshadows_cons_t[] = {{0, "MIN"}, {4, "MAX"}, {0, NULL}};
consvar_t cv_grcubeshadows     = {"gr_cubeshadows",     "1",   CV_SAVE, cubeshadows_cons_t, NULL, 0};

// Screen-space reflections (Phase 4.1).
static void CV_GrSSR_OnChange(); // forward-declared so cv_grssr can reference it
extern consvar_t cv_grssr;       // forward-declared so the callback can reference it
static void CV_GrSSR_OnChange()
{
    if (cv_grssr.value && !cv_grdeferred.value)
    {
        cv_grssr.Set(0);
        CONS_Printf("gr_ssr requires gr_deferred On\n");
    }
}
consvar_t cv_grssr             = {"gr_ssr",             "Off", CV_SAVE | CV_CALL, CV_OnOff, CV_GrSSR_OnChange, 0};
static CV_PossibleValue_t ssrstrength_cons_t[] = {{0, "MIN"}, {100, "MAX"}, {0, NULL}};
consvar_t cv_grssrstrength     = {"gr_ssrstrength",     "30",  CV_SAVE, ssrstrength_cons_t, NULL, 0};

// Volumetric fog PostFX pass (Phase 4.2).
// Use gr_fog 0 when enabling this to avoid double-fogging.
consvar_t cv_grvolfog          = {"gr_volfog",          "Off", CV_SAVE, CV_OnOff, NULL, 0};

// FXAA anti-aliasing (Phase 5.2).
consvar_t cv_grfxaa            = {"gr_fxaa",            "Off", CV_SAVE, CV_OnOff, NULL, 0};

// God rays — screen-space volumetric light shafts (Phase 5.3).
consvar_t cv_grgodrays         = {"gr_godrays",         "Off", CV_SAVE, CV_OnOff, NULL, 0};
consvar_t cv_grgodraysstrength = {"gr_godraysstrength", "0.5", CV_SAVE | CV_FLOAT, NULL, NULL, 0};
static CV_PossibleValue_t sunazimuth_cons_t[]  = {{0, "MIN"}, {359, "MAX"}, {0, NULL}};
static CV_PossibleValue_t sunaltitude_cons_t[] = {{0, "MIN"}, {89,  "MAX"}, {0, NULL}};
consvar_t cv_grsunazimuth      = {"gr_sunazimuth",      "225", CV_SAVE, sunazimuth_cons_t,  NULL, 0};
consvar_t cv_grsunaltitude     = {"gr_sunaltitude",     "45",  CV_SAVE, sunaltitude_cons_t, NULL, 0};

// Deferred Blinn-Phong specular (Phase 5.1). Only visible when gr_deferred is on.
static void CV_GrSpecular_OnChange(); // forward-declared so cv_grspecular can reference it
extern consvar_t cv_grspecular;       // forward-declared so the callback can reference it
static void CV_GrSpecular_OnChange()
{
    if (cv_grspecular.value && !cv_grdeferred.value)
    {
        cv_grspecular.Set(0);
        CONS_Printf("gr_specular requires gr_deferred On\n");
    }
}
consvar_t cv_grspecular        = {"gr_specular",        "0.3", CV_SAVE | CV_FLOAT | CV_CALL, NULL, CV_GrSpecular_OnChange, 0};
static CV_PossibleValue_t specexp_cons_t[] = {{4, "MIN"}, {128, "MAX"}, {0, NULL}};
consvar_t cv_grspecularexp     = {"gr_specularexp",     "32",  CV_SAVE, specexp_cons_t, NULL, 0};

void OGL_AddCommands()
{
    cv_grgammared.Reg();
    cv_grgammagreen.Reg();
    cv_grgammablue.Reg();

    cv_grfiltermode.Reg();
    cv_granisotropy.Reg();

    cv_grnearclippingplane.Reg();
    cv_grfarclippingplane.Reg();

    cv_grdynamiclighting.Reg();
    cv_grcoronas.Reg();
    cv_grcoronasize.Reg();

    cv_grfog.Reg();
    cv_grfogdensity.Reg();
    cv_grfogcolor.Reg();
    cv_grnormalmapstrength.Reg();

    cv_glquality.Reg();
    cv_fpslimit.Reg();

    cv_grbloom.Reg();
    cv_grbloomthreshold.Reg();
    cv_grbloomstrength.Reg();
    cv_grssao.Reg();
    cv_grssaostrength.Reg();
    cv_grdeferred.Reg();
    cv_grcubeshadows.Reg();
    cv_grssr.Reg();
    cv_grssrstrength.Reg();
    cv_grvolfog.Reg();
    cv_grfxaa.Reg();
    cv_grgodrays.Reg();
    cv_grgodraysstrength.Reg();
    cv_grsunazimuth.Reg();
    cv_grsunaltitude.Reg();
    cv_grspecular.Reg();
    cv_grspecularexp.Reg();
}

// Converts Doom sector light values to suitable background pixel
// color. extralight is for temporary brightening of the screen due to
// muzzle flashes etc.
byte LightLevelToLum(int l, int extralight)
{
    /*
    if (fixedcolormap)
      return 255;
    */
    l = lightleveltonumlut[l];
    l += (extralight << 4);
    if (l > 255)
        l = 255;
    return l;
}

// Hurdler's magical mystery mapping function initializer.
void InitLumLut()
{
    for (int i = 0; i < 256; i++)
    {
        // this polygone is the solution of equ : f(0)=0, f(1)=1
        // f(.5)=.5, f'(0)=0, f'(1)=0), f'(.5)=K
#define K 2
#define A (-24 + 16 * K)
#define B (60 - 40 * K)
#define C (32 * K - 50)
#define D (-8 * K + 15)
        float x = (float)i / 255;
        float xx, xxx;
        xx = x * x;
        xxx = x * xx;
        float k = 255 * (A * xx * xxx + B * xx * xx + C * xxx + D * xx);
        lightleveltonumlut[i] = 255 < k ? 255 : int(k); // min(255, k);
    }
}

/// Tells whether the spesified extension is supported by the current
/// OpenGL implementation.

bool GLExtAvailable(const char *extension)
{
    const GLubyte *start;
    GLubyte *where, *terminator;

    where = (GLubyte *)strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;

    // Always re-query: the pointer is context-specific and becomes stale after a context switch.
    gl_extensions = glGetString(GL_EXTENSIONS);

    start = gl_extensions;
    for (;;)
    {
        where = (GLubyte *)strstr((const char *)start, extension);
        if (!where)
            break;
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return true;
        start = terminator;
    }
    return false;
}

// This function runs some unit and sanity tests to the plane geometry
// & other routines.

void GeometryUnitTests()
{
    CONS_Printf("Starting geometry unit tests.\n");

    bbox_t bb;
    bb.Set(0.5, 0.5, 0.5);

    divline_t l;
    l.x = 0.5;
    l.y = -0.5;
    l.dx = 1.0;
    l.dy = 1.5;

    // Point on line side.
    if (l.PointOnSide(0, 0) == l.PointOnSide(1, 0))
        CONS_Printf("Point on line side test #1 failed.\n");

    if (l.PointOnSide(1, 0) == l.PointOnSide(1, 1))
        CONS_Printf("Point on line side test #2 failed.\n");

    // Inspect line crossings.
    /*
    if(!P_LinesegsCross(0, 0, 1, 1, 0, 1, 1, 0))
      CONS_Printf("Crossing test #1 failed.\n");

    if(!P_LinesegsCross(0, 0, 0, 1, -0.5, 0.5, 0.5, 0.5))
      CONS_Printf("Crossing test #2 failed.\n");

    if(P_LinesegsCross(0, 0, 1, 1, 2, 0, 2, 1))
      CONS_Printf("Crossing test #3 failed.\n");

    if(!P_LinesegsCross(0, 0, 1, 0, 0.5, -0.5, 1.5, 1))
      CONS_Printf("Crossing test #4 failed.\n");

    if(!P_LinesegsCross(1, 0, 1, 1, 0.5, -0.5, 1.5, 1))
      CONS_Printf("Crossing test #5 failed.\n");
    */

    // Bounding boxes.
    if (!bb.LineCrossesEdge(-2, 0, 1000, 0.5))
        CONS_Printf("Bounding box test #1 failed.\n");

    if (!bb.LineCrossesEdge(-1, 0.5, 10, 0.5))
        CONS_Printf("Bounding box test #2 failed.\n");

    if (!bb.LineCrossesEdge(0.5, -1, 0.5, 100))
        CONS_Printf("Bounding box test #3 failed.\n");

    if (bb.LineCrossesEdge(0.2, 0.2, 0.7, 0.7))
        CONS_Printf("Bounding box test #4 failed.\n");

    if (!bb.LineCrossesEdge(0.5, -0.5, 1.5, 1))
        CONS_Printf("Bounding box test #5 failed.\n");

    CONS_Printf("Geometry unit tests finished.\n");
}
