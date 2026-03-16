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

// VSync CVar
static CV_PossibleValue_t vsync_cons_t[] = {{0,"Off"},{1,"On"},{-1,"Adaptive"},{0,NULL}};
#ifdef SDL2
static void CV_VSync_OnChange();
#endif
consvar_t cv_vsync = {"vsync","On", CV_SAVE|CV_CALL, vsync_cons_t,
#ifdef SDL2
    CV_VSync_OnChange
#else
    NULL
#endif
};
#ifdef SDL2
static void CV_VSync_OnChange()
{
    SDL_GL_SetSwapInterval(cv_vsync.value);
}
#endif

// MSAA CVar (applied before GL context creation; changing at runtime requires a vid restart)
static CV_PossibleValue_t msaa_cons_t[] = {{0,"Off"},{2,"2x"},{4,"4x"},{8,"8x"},{0,NULL}};
consvar_t cv_msaa = {"msaa","0", CV_SAVE, msaa_cons_t, NULL};

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
static SDL_Renderer *sdlRenderer = NULL;
static SDL_Texture *sdlTexture = NULL;
static SDL_DisplayMode vidInfo_mode;
#define vidInfo (&vidInfo_mode)
#else
static const SDL_VideoInfo *vidInfo = NULL;
#endif
static SDL_Surface *vidSurface = NULL;
static SDL_Color localPalette[256];

// Export window pointer for other modules (like i_system.cpp).
// In GL mode the active window is owned by oglrenderer, not sdlWindow.
#ifdef SDL2
SDL_Window* GetSDLWindow()
{
    if (rendermode == render_opengl && oglrenderer)
        return oglrenderer->GetScreen();
    return sdlWindow;
}
#endif

const static Uint32 surfaceFlags = SDL_SWSURFACE | SDL_HWPALETTE;

struct vidmode_t
{
    int w, h;
    char name[16];
};


// windowed video modes (modern list including widescreen and 4K)
static vidmode_t windowedModes[] = {
    {320,  200 },
    {640,  480 },
    {800,  600 },
    {1024, 768 },
    {1280, 720 },
    {1280, 1024},
    {1366, 768 },
    {1600, 900 },
    {1920, 1080},
    {2560, 1080},
    {2560, 1440},
    {3440, 1440},
    {3840, 2160},
};
#define MAXWINMODES (int)(sizeof(windowedModes)/sizeof(windowedModes[0]))

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
        // In SDL2 software mode, convert indexed surface to ARGB and render
        if (vidSurface && sdlRenderer && sdlTexture)
        {
            // Lock surface if needed
            if (SDL_MUSTLOCK(vidSurface) && vidSurface->locked == 0)
                SDL_LockSurface(vidSurface);

            // Get the surface pixels
            Uint8 *srcPixels = (Uint8 *)vidSurface->pixels;
            int pitch = vidSurface->pitch;

            // We need to convert indexed8 to ARGB8888
            // Create a temporary buffer for conversion if needed
            static Uint32 *convertBuffer = NULL;
            static int convertWidth = 0;
            static int convertHeight = 0;

            int width = vid.width;
            int height = vid.height;

            // Reallocate buffer if size changed
            if (convertBuffer == NULL || convertWidth != width || convertHeight != height)
            {
                free(convertBuffer);
                convertBuffer = (Uint32 *)malloc(width * height * sizeof(Uint32));
                convertWidth = width;
                convertHeight = height;
            }

            // Convert indexed to ARGB using the palette
            for (int y = 0; y < height; y++)
            {
                Uint8 *srcRow = srcPixels + y * pitch;
                Uint32 *dstRow = convertBuffer + y * width;
                for (int x = 0; x < width; x++)
                {
                    Uint8 index = srcRow[x];
                    SDL_Color col = localPalette[index];
                    dstRow[x] = (col.a << 24) | (col.r << 16) | (col.g << 8) | col.b;
                }
            }

            SDL_UpdateTexture(sdlTexture, NULL, convertBuffer, width * sizeof(Uint32));
            SDL_RenderClear(sdlRenderer);
            SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
            SDL_RenderPresent(sdlRenderer);

            if (SDL_MUSTLOCK(vidSurface))
                SDL_UnlockSurface(vidSurface);
        }
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
        localPalette[i].a = 255; // Fully opaque
    }

    SDL_SetColors(vidSurface, localPalette, 0, 256);
}

void I_SetGamma(float r, float g, float b)
{
    SDL_SetGamma(r, g, b);
}

// All three display modes (Windowed/Fullscreen/Borderless) share the same resolution list.
int I_NumVideoModes()
{
    return MAXWINMODES;
}

const char *I_GetVideoModeName(unsigned n)
{
    if (n >= (unsigned)MAXWINMODES)
        return NULL;
    return windowedModes[n].name;
}

int I_GetVideoModeForSize(int w, int h)
{
    for (int i = 0; i < MAXWINMODES; i++)
    {
        if (windowedModes[i].w == w && windowedModes[i].h == h)
            return i;
    }
    // No exact match — return the closest resolution by area
    int best = 0;
    long bestDiff = LONG_MAX;
    long target = (long)w * h;
    for (int i = 0; i < MAXWINMODES; i++)
    {
        long diff = labs((long)windowedModes[i].w * windowedModes[i].h - target);
        if (diff < bestDiff)
        {
            bestDiff = diff;
            best = i;
        }
    }
    return best;
}

int I_SetVideoMode(int modeNum)
{
    Uint32 flags = surfaceFlags;
    vid.modenum = modeNum;

#ifdef SDL2
    int dispmode = cv_fullscreen.value; // 0=Windowed, 1=Fullscreen, 2=Borderless

    if (dispmode == 2)
    {
        // Borderless: always use desktop resolution
        SDL_DisplayMode dm;
        SDL_GetDesktopDisplayMode(0, &dm);
        vid.width  = dm.w;
        vid.height = dm.h;
        CONS_Printf("I_SetVideoMode: borderless %d x %d\n", vid.width, vid.height);
    }
    else
    {
        vid.width  = windowedModes[modeNum].w;
        vid.height = windowedModes[modeNum].h;
        const char *tag = (dispmode == 1) ? "fullscreen" : "windowed";
        CONS_Printf("I_SetVideoMode: %s %d x %d (%d bpp)\n",
                    tag, vid.width, vid.height, vid.BitsPerPixel);
    }

    if (rendermode == render_soft)
    {
        // Destroy old window, renderer, texture and surface
        if (vidSurface)
        {
            SDL_FreeSurface(vidSurface);
            vidSurface = NULL;
        }
        if (sdlTexture)
        {
            SDL_DestroyTexture(sdlTexture);
            sdlTexture = NULL;
        }
        if (sdlRenderer)
        {
            SDL_DestroyRenderer(sdlRenderer);
            sdlRenderer = NULL;
        }
        if (sdlWindow)
        {
            SDL_DestroyWindow(sdlWindow);
            sdlWindow = NULL;
        }

        // Create new window
        Uint32 windowFlags = SDL_WINDOW_SHOWN;
        if (dispmode == 1)
            windowFlags |= SDL_WINDOW_FULLSCREEN;
        else if (dispmode == 2)
            windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

        sdlWindow = SDL_CreateWindow("Doom Legacy",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            vid.width, vid.height, windowFlags);

        if (sdlWindow == NULL)
            I_Error("Could not set vidmode: %s\n", SDL_GetError());

        // Create renderer for software rendering
        sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
        if (sdlRenderer == NULL)
            I_Error("Could not create renderer: %s\n", SDL_GetError());

        // Create texture for rendering
        sdlTexture = SDL_CreateTexture(sdlRenderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            vid.width, vid.height);
        if (sdlTexture == NULL)
            I_Error("Could not create texture: %s\n", SDL_GetError());

        // Create surface for software rendering (for direct pixel access)
        // Use 8-bit indexed format for DOOM's paletted graphics
        // Note: SDL2 renderer doesn't directly support 8-bit, so we convert in I_FinishUpdate
        vidSurface = SDL_CreateRGBSurfaceWithFormat(0, vid.width, vid.height, 8,
            SDL_PIXELFORMAT_INDEX8);

        if (vidSurface == NULL)
            I_Error("Could not create surface: %s\n", SDL_GetError());

        if (vidSurface->pixels == NULL)
            I_Error("Didn't get a valid pixels pointer (SDL). Exiting.\n");

        vid.direct = static_cast<byte *>(vidSurface->pixels);
        vid.direct[0] = 1;
    }
    else
    {
        if (!oglrenderer->InitVideoMode(vid.width, vid.height, dispmode))
            I_Error("Could not set OpenGL vidmode.\n");
    }

    // Flush stale SDL events (resize, focus, etc.) generated during window
    // creation so they don't confuse the input system this frame.
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

    I_StartupMouse(); // grabs mouse and keyboard input if necessary

    return 1;
#else
    // Original SDL1 code (fullscreen/windowed only; borderless not supported on SDL1)
    vid.width  = windowedModes[modeNum].w;
    vid.height = windowedModes[modeNum].h;

    if (cv_fullscreen.value == 1)
    {
        flags |= SDL_FULLSCREEN;
        CONS_Printf("I_SetVideoMode: fullscreen %d x %d (%d bpp)\n",
                    vid.width, vid.height, vid.BitsPerPixel);
    }
    else
    {
        CONS_Printf("I_SetVideoMode: windowed %d x %d (%d bpp)\n",
                    vid.width, vid.height, vid.BitsPerPixel);
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
        vid.direct[0] = 1;
    }
    else
    {
        if (!oglrenderer->InitVideoMode(vid.width, vid.height, dispmode))
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

    // Name the unified mode list
    for (int n = 0; n < MAXWINMODES; n++)
        sprintf(windowedModes[n].name, "%dx%d", windowedModes[n].w, windowedModes[n].h);

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
        if (cv_fullscreen.value == 1)
            flags |= SDL_WINDOW_FULLSCREEN;
        else if (cv_fullscreen.value == 2)
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

        sdlWindow = SDL_CreateWindow("Doom Legacy",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            vid.width, vid.height, flags);

        if (sdlWindow == NULL)
        {
            CONS_Printf("Could not create window: %s\n", SDL_GetError());
            return false;
        }

        // Create renderer for software rendering
        sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
        if (sdlRenderer == NULL)
        {
            CONS_Printf("Could not create renderer: %s\n", SDL_GetError());
            return false;
        }

        // Create texture for rendering
        sdlTexture = SDL_CreateTexture(sdlRenderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            vid.width, vid.height);
        if (sdlTexture == NULL)
        {
            CONS_Printf("Could not create texture: %s\n", SDL_GetError());
            return false;
        }

        // Create surface for software rendering (for direct pixel access)
        // Use 8-bit indexed format for DOOM's paletted graphics
        vidSurface = SDL_CreateRGBSurfaceWithFormat(0, vid.width, vid.height, 8,
            SDL_PIXELFORMAT_INDEX8);

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

    // Name the unified mode list
    for (int n = 0; n < MAXWINMODES; n++)
        sprintf(windowedModes[n].name, "%dx%d", windowedModes[n].w, windowedModes[n].h);

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
        if (sdlTexture)
        {
            SDL_DestroyTexture(sdlTexture);
            sdlTexture = NULL;
        }
        if (sdlRenderer)
        {
            SDL_DestroyRenderer(sdlRenderer);
            sdlRenderer = NULL;
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
