// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ogl_sdl.cpp 514 2007-12-21 16:07:36Z smite-meister $
//
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
/// \brief SDL specific part of the OpenGL API for Doom Legacy

#include "SDL.h"

#ifdef SDL2
#include <SDL2/SDL_video.h>
#endif

#include "command.h"
#include "cvars.h"
#include "hardware/hwr_render.h"
#include "screen.h"
#include "v_video.h"

#ifdef SDL2
static SDL_Window *oglWindow = NULL;
static SDL_GLContext glContext = NULL;
#else
static SDL_Surface *vidSurface = NULL; // use the one from i_video_sdl.c instead?
#endif

bool OglSdlSurface()
{
#ifdef SDL2
    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

    if (cv_fullscreen.value == 1)
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    else if (cv_fullscreen.value == 2)
        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    // Clean up old window
    if (oglWindow)
    {
        SDL_DestroyWindow(oglWindow);
        oglWindow = NULL;
    }
    if (glContext)
    {
        SDL_GL_DeleteContext(glContext);
        glContext = NULL;
    }

    // We want at least 1 bit (???) for R, G, and B, and at least 16 bits for depth buffer.
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    oglWindow = SDL_CreateWindow("Doom Legacy",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        vid.width, vid.height, windowFlags);

    if (!oglWindow)
        return false;

    glContext = SDL_GL_CreateContext(oglWindow);
    if (!glContext)
        return false;

    CONS_Printf("HWRend::Startup(): %dx%d OpenGL\n", vid.width, vid.height);

    return true;
#else
    // Original SDL1 code
    Uint32 surfaceFlags;

    if (NULL != vidSurface)
    {
        SDL_FreeSurface(vidSurface);
        vidSurface = NULL;
#ifdef VOODOOSAFESWITCHING
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_InitSubSystem(SDL_INIT_VIDEO);
#endif
    }

    if (cv_fullscreen.value == 1)
        surfaceFlags = SDL_OPENGL | SDL_FULLSCREEN;
    else
        surfaceFlags = SDL_OPENGL;

    // We want at least 1 bit (???) for R, G, and B, and at least 16 bits for depth buffer.
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    int cbpp = SDL_VideoModeOK(vid.width, vid.height, vid.BitsPerPixel, surfaceFlags);
    if (cbpp < 16)
        return false;
    if ((vidSurface = SDL_SetVideoMode(vid.width, vid.height, cbpp, surfaceFlags)) == NULL)
        return false;

    CONS_Printf("HWRend::Startup(): %dx%d %d bits\n", vid.width, vid.height, cbpp);

    return true;
#endif
}

void OglSdlFinishUpdate(bool vidwait)
{
#ifdef SDL2
    if (oglWindow)
        SDL_GL_SwapWindow(oglWindow);
#else
    SDL_GL_SwapBuffers();
#endif
}

void OglSdlShutdown()
{
#ifdef SDL2
    if (glContext)
    {
        SDL_GL_DeleteContext(glContext);
        glContext = NULL;
    }
    if (oglWindow)
    {
        SDL_DestroyWindow(oglWindow);
        oglWindow = NULL;
    }
#else
    if (NULL != vidSurface)
    {
        SDL_FreeSurface(vidSurface);
        vidSurface = NULL;
    }
#endif
}

void OglSdlSetGamma(float r, float g, float b)
{
#ifdef SDL2
    // SDL2 removed SDL_SetGamma, need to use OpenGL directly
    // For now, this is a no-op
#else
    SDL_SetGamma(r, g, b);
#endif
}

/*
void OglSdlSetPalette(RGBA_t *palette, RGBA_t *gamma)
{
    int i;

    for (i=0; i<256; i++) {
        myPaletteData[i].s.red   = MIN((palette[i].s.red   * gamma->s.red)  /127, 255);
        myPaletteData[i].s.green = MIN((palette[i].s.green * gamma->s.green)/127, 255);
        myPaletteData[i].s.blue  = MIN((palette[i].s.blue  * gamma->s.blue) /127, 255);
        myPaletteData[i].s.alpha = palette[i].s.alpha;
    }
    // on a chang� de palette, il faut recharger toutes les textures
    // jaja, und noch viel mehr ;-)
    Flush();
}
*/
