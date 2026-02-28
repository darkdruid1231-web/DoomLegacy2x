// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_video.cpp 536 2009-06-29 06:46:13Z smite-meister $
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

/// \file
/// \brief SDL video interface

#include <stdlib.h>
#include <string.h>
#include <vector>

#include "SDL.h"

#ifdef SDL2
#include <SDL2/SDL_video.h>
// SDL2 compatibility macros
#define SDL_VideoInfo SDL_DisplayMode
#define SDL_HWPALETTE 0
#define SDL_FULLSCREEN SDL_WINDOW_FULLSCREEN
#define SDL_SetVideoMode(w, h, b, flags) SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags)
#define SDL_Flip(surface) // SDL2 uses SDL_RenderPresent instead
#define SDL_SetColors(surface, colors, first, n) 0
#define SDL_SetGamma(r, g, b) // SDL2 removed this
#define SDL_GetVideoInfo() SDL_GetDesktopDisplayMode()
#define vidInfo (*vidInfo_ptr)
static SDL_DisplayMode* vidInfo_ptr = NULL;
#define SDL_SWSURFACE 0
#define SDL_HWPALETTE 0
#endif

#include "command.h"
#include "doomdef.h"

#include "i_system.h"
#include "i_video.h"

#include "m_argv.h"
#include "screen.h"

#include "m_dll.h"

#include "hardware/oglhelpers.hpp"
#include "hardware/oglrenderer.hpp"

extern consvar_t cv_fullscreen; // for fullscreen support

// globals
OGLRenderer *oglrenderer = NULL;
rendermode_t rendermode = render_soft;

bool graphics_started = false;

#ifdef DYNAMIC_LINKAGE
static LegacyDLL OGL_renderer;
#endif

// SDL vars
#ifdef SDL2
static SDL_Window *sdlWindow = NULL;
static SDL_DisplayMode vidInfo_mode;
#define vidInfo (&vidInfo_mode)
#else
static const SDL_VideoInfo *vidInfo = NULL;
#endif
static SDL_Surface *vidSurface = NULL;
static SDL_Color localPalette[256];

// Export window pointer for other modules (like i_system.cpp)
#ifdef SDL2
SDL_Window* GetSDLWindow() { return sdlWindow; }
#endif

const static Uint32 surfaceFlags = SDL_SWSURFACE | SDL_HWPALETTE;

// maximum number of windowed modes (see windowedModes[][])
#if !defined(__MACOS__) && !defined(__APPLE_CC__)
#define MAXWINMODES (9)
#else
#define MAXWINMODES (12)
#endif

struct vidmode_t
{
    int w, h;
    char name[16];
};

static std::vector<vidmode_t> fullscrModes;

// windowed video modes from which to choose from.
static vidmode_t windowedModes[MAXWINMODES] = {
#ifdef __APPLE_CC__
    {MAXVIDWIDTH /*1600*/, MAXVIDHEIGHT /*1200*/},
    {1440, 900}, /* iMac G5 native res */
    {1280, 1024},
    {1152, 720}, /* iMac G5 native res */
    {1024, 768},
    {1024, 640},
    {800, 600},
    {800, 500},
    {640, 480},
    {512, 384},
    {400, 300},
    {320, 200}
#else
    {MAXVIDWIDTH /*1600*/, MAXVIDHEIGHT /*1200*/},
    {1280, 1024}, // 1.25
    {1024, 768},  // 1.3_
    {800, 600},   // 1.3_
    {640, 480},   // 1.3_
    {640, 400},   // 1.6
    {512, 384},   // 1.3_
    {400, 300},   // 1.3_
    {320, 200}    // original Doom resolution (pixel ar 1.6), meant for 1.3_ aspect ratio monitors
                  // (nonsquare pixels!)
#endif
};

//
// I_StartFrame
//
void I_StartFrame()
{
    if (render_soft == rendermode)
    {
        if (SDL_MUSTLOCK(vidSurface))
        {
            if (SDL_LockSurface(vidSurface) < 0)
                return;
        }
    }
    else
    {
        oglrenderer->StartFrame();
    }
}

//
// I_OsPolling
//
void I_OsPolling()
{
    if (!graphics_started)
        return;

    I_GetEvent();
}

//
// I_FinishUpdate
//
void I_FinishUpdate()
{
    if (rendermode == render_soft)
    {
#ifdef SDL2
        // In SDL2, we need to blit the surface to the window
        // For now, just flip - actual rendering happens elsewhere
        // TODO: Implement proper SDL_RenderCopy for software mode
#else
        /*
        if (vid.screens[0] != vid.direct)
      memcpy(vid.direct, vid.screens[0], vid.height*vid.rowbytes);
        */
        SDL_Flip(vidSurface);

        if (SDL_MUSTLOCK(vidSurface))
            SDL_UnlockSurface(vidSurface);
#endif
    }
    else if (oglrenderer != NULL)
        oglrenderer->FinishFrame();

    I_GetEvent();
}

//
// I_SetPalette
//
void I_SetPalette(RGB_t *palette)
{
    for (int i = 0; i < 256; i++)
    {
        localPalette[i].r = palette[i].r;
        localPalette[i].g = palette[i].g;
        localPalette[i].b = palette[i].b;
    }

    SDL_SetColors(vidSurface, localPalette, 0, 256);
}

void I_SetGamma(float r, float g, float b)
{
    SDL_SetGamma(r, g, b);
}

// return number of fullscreen or windowed modes
int I_NumVideoModes()
{
    if (cv_fullscreen.value)
        return fullscrModes.size();
    else
        return MAXWINMODES;
}

const char *I_GetVideoModeName(unsigned n)
{
    if (cv_fullscreen.value)
    {
        if (n >= fullscrModes.size())
            return NULL;

        return fullscrModes[n].name;
    }

    // windowed modes
    if (n >= MAXWINMODES)
        return NULL;

    return windowedModes[n].name;
}

int I_GetVideoModeForSize(int w, int h)
{
    int matchMode = -1;

    if (cv_fullscreen.value)
    {
        for (unsigned i = 0; i < fullscrModes.size(); i++)
        {
            if (fullscrModes[i].w == w && fullscrModes[i].h == h)
            {
                matchMode = i;
                break;
            }
        }

        if (matchMode == -1) // use smallest mode
            matchMode = fullscrModes.size() - 1;
    }
    else
    {
        for (unsigned i = 0; i < MAXWINMODES; i++)
        {
            if (windowedModes[i].w == w && windowedModes[i].h == h)
            {
                matchMode = i;
                break;
            }
        }

        if (matchMode == -1) // use smallest mode
            matchMode = MAXWINMODES - 1;
    }

    return matchMode;
}

int I_SetVideoMode(int modeNum)
{
    Uint32 flags = surfaceFlags;
    vid.modenum = modeNum;

#ifdef SDL2
    if (cv_fullscreen.value)
    {
        vid.width = fullscrModes[modeNum].w;
        vid.height = fullscrModes[modeNum].h;
        flags |= SDL_WINDOW_FULLSCREEN;

        CONS_Printf("I_SetVideoMode: fullscreen %d x %d (%d bpp)\n",
                    vid.width,
                    vid.height,
                    vid.BitsPerPixel);
    }
    else
    {
        vid.width = windowedModes[modeNum].w;
        vid.height = windowedModes[modeNum].h;

        CONS_Printf(
            "I_SetVideoMode: windowed %d x %d (%d bpp)\n", vid.width, vid.height, vid.BitsPerPixel);
    }

    if (rendermode == render_soft)
    {
        // Destroy old window and surface
        if (vidSurface)
        {
            SDL_FreeSurface(vidSurface);
            vidSurface = NULL;
        }
        if (sdlWindow)
        {
            SDL_DestroyWindow(sdlWindow);
            sdlWindow = NULL;
        }

        // Create new window
        Uint32 windowFlags = SDL_WINDOW_SHOWN;
        if (cv_fullscreen.value)
            windowFlags |= SDL_WINDOW_FULLSCREEN;

        sdlWindow = SDL_CreateWindow("Doom Legacy",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            vid.width, vid.height, windowFlags);

        if (sdlWindow == NULL)
            I_Error("Could not set vidmode: %s\n", SDL_GetError());

        // Create surface for software rendering
        vidSurface = SDL_CreateRGBSurface(0, vid.width, vid.height, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

        if (vidSurface == NULL)
            I_Error("Could not create surface: %s\n", SDL_GetError());

        if (vidSurface->pixels == NULL)
            I_Error("Didn't get a valid pixels pointer (SDL). Exiting.\n");

        vid.direct = static_cast<byte *>(vidSurface->pixels);
        vid.direct[0] = 1;
    }
    else
    {
        if (!oglrenderer->InitVideoMode(vid.width, vid.height, cv_fullscreen.value))
            I_Error("Could not set OpenGL vidmode.\n");
    }

    I_StartupMouse(); // grabs mouse and keyboard input if necessary

    return 1;
#else
    // Original SDL1 code
    if (cv_fullscreen.value)
    {
        vid.width = fullscrModes[modeNum].w;
        vid.height = fullscrModes[modeNum].h;
        flags |= SDL_FULLSCREEN;

        CONS_Printf("I_SetVideoMode: fullscreen %d x %d (%d bpp)\n",
                    vid.width,
                    vid.height,
                    vid.BitsPerPixel);
    }
    else
    { // !cv_fullscreen.value
        vid.width = windowedModes[modeNum].w;
        vid.height = windowedModes[modeNum].h;

        CONS_Printf(
            "I_SetVideoMode: windowed %d x %d (%d bpp)\n", vid.width, vid.height, vid.BitsPerPixel);
    }

    if (rendermode == render_soft)
    {
        SDL_FreeSurface(vidSurface);

        vidSurface = SDL_SetVideoMode(vid.width, vid.height, vid.BitsPerPixel, flags);
        if (vidSurface == NULL)
            I_Error("Could not set vidmode\n");

        if (vidSurface->pixels == NULL)
            I_Error("Didn't get a valid pixels pointer (SDL). Exiting.\n");

        vid.direct = static_cast<byte *>(vidSurface->pixels);
        // VB: FIXME this stops execution at the latest
        vid.direct[0] = 1;
    }
    else
    {
        if (!oglrenderer->InitVideoMode(vid.width, vid.height, cv_fullscreen.value))
            I_Error("Could not set OpenGL vidmode.\n");
    }

    I_StartupMouse(); // grabs mouse and keyboard input if necessary

    return 1;
#endif
}

bool I_StartupGraphics()
{
    if (graphics_started)
        return true;

#ifdef SDL2
    // Get desktop display mode for reference
    if (SDL_GetDesktopDisplayMode(0, &vidInfo_mode) < 0)
    {
        CONS_Printf("Could not get desktop display mode: %s\n", SDL_GetError());
        return false;
    }

    // Get available fullscreen modes
    int numModes = SDL_GetNumDisplayModes(0);
    for (int i = 0; i < numModes; i++)
    {
        SDL_DisplayMode mode;
        if (SDL_GetDisplayMode(0, i, &mode) == 0)
        {
            if (mode.w <= MAXVIDWIDTH && mode.h <= MAXVIDHEIGHT)
            {
                vidmode_t temp;
                temp.w = mode.w;
                temp.h = mode.h;
                sprintf(temp.name, "%dx%d", temp.w, temp.h);

                // Avoid duplicates
                bool found = false;
                for (unsigned j = 0; j < fullscrModes.size(); j++)
                {
                    if (fullscrModes[j].w == temp.w && fullscrModes[j].h == temp.h)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    fullscrModes.push_back(temp);
            }
        }
    }

    CONS_Printf(" Found %d video modes.\n", fullscrModes.size());
    if (fullscrModes.empty())
    {
        CONS_Printf(" No suitable video modes found!\n");
        return false;
    }

    // name the windowed modes
    for (int n = 0; n < MAXWINMODES; n++)
        sprintf(windowedModes[n].name, "win %dx%d", windowedModes[n].w, windowedModes[n].h);

    vid.BytesPerPixel = 1;
    vid.BitsPerPixel = 8;

    // default resolution
    vid.width = BASEVIDWIDTH;
    vid.height = BASEVIDHEIGHT;

    if (M_CheckParm("-opengl"))
    {
        // OpenGL mode
        rendermode = render_opengl;
        oglrenderer = new OGLRenderer;
    }
    else
    {
        // software mode - create window and use software renderer
        rendermode = render_soft;
        CONS_Printf("I_StartupGraphics: windowed %d x %d x %d bpp\n",
                    vid.width,
                    vid.height,
                    vid.BitsPerPixel);

        Uint32 flags = SDL_WINDOW_SHOWN;
        if (cv_fullscreen.value)
            flags |= SDL_WINDOW_FULLSCREEN;

        sdlWindow = SDL_CreateWindow("Doom Legacy",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            vid.width, vid.height, flags);

        if (sdlWindow == NULL)
        {
            CONS_Printf("Could not create window: %s\n", SDL_GetError());
            return false;
        }

        // For software rendering in SDL2, we need a renderer and texture
        // For now, create a surface for compatibility
        vidSurface = SDL_CreateRGBSurface(0, vid.width, vid.height, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

        if (vidSurface == NULL)
        {
            CONS_Printf("Could not create surface: %s\n", SDL_GetError());
            return false;
        }
        vid.direct = static_cast<byte *>(vidSurface->pixels);
    }

    SDL_ShowCursor(SDL_DISABLE);
    I_StartupMouse(); // grab mouse and keyboard input if needed

    graphics_started = true;

    return true;
#else
    // Original SDL1 code
    // Get video info for screen resolutions
    vidInfo = SDL_GetVideoInfo();
    // now we _could_ do all kinds of cool tests to determine which
    // video modes are available, but...
    // vidInfo->vfmt is the pixelformat of the "best" video mode available

    // CONS_Printf("Bpp = %d, bpp = %d\n", vidInfo->vfmt->BytesPerPixel,
    // vidInfo->vfmt->BitsPerPixel);

    // list all available video modes corresponding to the "best" pixelformat
    SDL_Rect **modeList = SDL_ListModes(NULL, SDL_FULLSCREEN | surfaceFlags);

    if (modeList == NULL)
    {
        CONS_Printf(" No video modes present!\n");
        return false;
    }
    else if (modeList == reinterpret_cast<SDL_Rect **>(-1))
    {
        CONS_Printf(" Unexpected: any video resolution is available in fullscreen mode.\n");
        return false;
    }

    // copy suitable modes to our own list
    int n = 0;
    while (modeList[n])
    {
        if (modeList[n]->w <= MAXVIDWIDTH && modeList[n]->h <= MAXVIDHEIGHT)
        {
            vidmode_t temp;
            temp.w = modeList[n]->w;
            temp.h = modeList[n]->h;
            sprintf(temp.name, "%dx%d", temp.w, temp.h);

            fullscrModes.push_back(temp);
            // CONS_Printf("  %s\n", temp.name);
        }
        n++;
    }

    CONS_Printf(" Found %d video modes.\n", fullscrModes.size());
    if (fullscrModes.empty())
    {
        CONS_Printf(" No suitable video modes found!\n");
        return false;
    }

    // name the windowed modes
    for (n = 0; n < MAXWINMODES; n++)
        sprintf(windowedModes[n].name, "win %dx%d", windowedModes[n].w, windowedModes[n].h);

        // even if I set vid.bpp and highscreen properly it does seem to
        // support only 8 bit  ...  strange
        // so lets force 8 bit (software mode only)
        // TODO why not use hicolor in sw mode too? it must work...

#if defined(__APPLE_CC__) || defined(__MACOS__)
    vid.BytesPerPixel = vidInfo->vfmt->BytesPerPixel;
    vid.BitsPerPixel = vidInfo->vfmt->BitsPerPixel;
    if (!M_CheckParm("-opengl"))
    {
        // software mode
        vid.BytesPerPixel = 1;
        vid.BitsPerPixel = 8;
    }
#else
    vid.BytesPerPixel = 1; // videoInfo->vfmt->BytesPerPixel
    vid.BitsPerPixel = 8;
#endif

    // default resolution
    vid.width = BASEVIDWIDTH;
    vid.height = BASEVIDHEIGHT;
    if (M_CheckParm("-opengl"))
    {
        // OpenGL mode
        rendermode = render_opengl;

#if 0 // FIXME: Hurdler: for now we do not use that anymore (but it should probably be back some day
#ifdef DYNAMIC_LINKAGE
      // dynamic linkage
      OGL_renderer.Open("r_opengl.dll");

      if (OGL_renderer.api_version != R_OPENGL_INTERFACE_VERSION)
        I_Error("r_opengl.dll interface version does not match with Legacy.exe!\n"
                "You must use the r_opengl.dll that came in the same distribution as your Legacy.exe.");

      hw_renderer_export_t *temp = (hw_renderer_export_t *)OGL_renderer.GetSymbol("r_export");
      memcpy(&HWD, temp, sizeof(hw_renderer_export_t));
      CONS_Printf("%s loaded.\n", OGL_renderer.name);
#else
      // static linkage
      memcpy(&HWD, &r_export, sizeof(hw_renderer_export_t));
#endif
#endif

        oglrenderer = new OGLRenderer;
    }
    else
    {
        // software mode
        rendermode = render_soft;
        CONS_Printf("I_StartupGraphics: windowed %d x %d x %d bpp\n",
                    vid.width,
                    vid.height,
                    vid.BitsPerPixel);
        vidSurface = SDL_SetVideoMode(vid.width, vid.height, vid.BitsPerPixel, surfaceFlags);

        if (vidSurface == NULL)
        {
            CONS_Printf("Could not set video mode!\n");
            return false;
        }
        vid.direct = static_cast<byte *>(vidSurface->pixels);
    }

    SDL_ShowCursor(SDL_DISABLE);
    I_StartupMouse(); // grab mouse and keyboard input if needed

    graphics_started = true;

    return true;
#endif
}

void I_ShutdownGraphics()
{
    // was graphics initialized anyway?
    if (!graphics_started)
        return;

#ifdef SDL2
    if (rendermode == render_soft)
    {
        if (vidSurface)
        {
            SDL_FreeSurface(vidSurface);
            vidSurface = NULL;
        }
        if (sdlWindow)
        {
            SDL_DestroyWindow(sdlWindow);
            sdlWindow = NULL;
        }
    }
    else
    {
        delete oglrenderer;
        oglrenderer = NULL;

#ifdef DYNAMIC_LINKAGE
        if (ogl_handle)
            CloseDLL(ogl_handle);
#endif
    }
#else
    if (rendermode == render_soft)
    {
        // vidSurface should be automatically freed
    }
    else
    {
        delete oglrenderer;
        oglrenderer = NULL;

#ifdef DYNAMIC_LINKAGE
        if (ogl_handle)
            CloseDLL(ogl_handle);
#endif
    }
#endif
    SDL_Quit();
}
