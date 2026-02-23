// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: oglrenderer.hpp 524 2008-01-20 23:48:31Z jussip $
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

#ifndef oglrenderer_hpp_
#define oglrenderer_hpp_

// Fix GCC 13+ / MinGW-w64 header conflicts
#ifdef __MINGW32__
// Prevent GCC from declaring conflicting _xgetbv and __cpuidex
// Let MinGW-w64's intrin-impl.h provide the compatible versions
# define _XSAVEINTRIN_H_INCLUDED 1
# define _XGETBV_DEFINED 1
# define __CPUID_H 1
# define _CPUID_H_INCLUDED 1
// Workaround for GCC 13 + MinGW-w64 __cpuidex redeclaration issue
# define __GNUC_GNU_INLINE__ 1
# ifdef __cpuidex
#  undef __cpuidex
# endif
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wattributes"
#endif

// SDL2 headers - use SDL2/SDL.h on MSYS2/MinGW or SDL/SDL.h fallback
// SDL1 code (USE_SDL2 not defined) falls back to same headers for compatibility
#ifdef USE_SDL2
#  include <SDL2/SDL.h>
#else
#  include <SDL/SDL.h>
#endif

#ifdef __MINGW32__
# pragma GCC diagnostic pop
#endif

// SDL1 -> SDL2 compatibility shims for legacy renderer code
// These map SDL1 constants to SDL2 equivalents when using SDL2
#ifndef SDL_SWSURFACE
#  define SDL_SWSURFACE 0
#endif
#ifndef SDL_OPENGL
#  define SDL_OPENGL SDL_WINDOW_OPENGL
#endif
#ifndef SDL_FULLSCREEN
#  define SDL_FULLSCREEN SDL_WINDOW_FULLSCREEN
#endif
#ifndef SDL_GL_DEPTH_SIZE
#  define SDL_GL_DEPTH_SIZE SDL_GL_DEPTH_SIZE
#endif
#ifndef SDL_GL_SwapBuffers
#  define SDL_GL_SwapBuffers SDL_GL_SwapWindow
#endif

#include<GL/gl.h>
#include<GL/glext.h>

// On Windows, <GL/gl.h> only covers OpenGL 1.1.  Declare the 1.4 point
// parameter functions explicitly — they are exported by opengl32.dll.
#if defined(_WIN32) && !defined(GL_POINT_PARAMETERS_DECLARED)
#  define GL_POINT_PARAMETERS_DECLARED 1
#  ifndef APIENTRY
#    define APIENTRY __stdcall
#  endif
   extern "C" void APIENTRY glPointParameterf(GLenum pname, GLfloat param);
   extern "C" void APIENTRY glPointParameterfv(GLenum pname, const GLfloat *params);
#endif

// SDL surface struct is always provided by SDL headers (1 or 2).
// Don't define a dummy - rely on SDL headers being present.
#include<GL/glu.h>

#include"vect.h"
#include"r_defs.h"

class PlayerInfo;
class Material;
class Actor;
struct fline_t;
struct quad;

/// \brief OpenGL renderer.
/*!
  This is an attempt at creating a new OpenGL renderer for Legacy.
  The basic idea is that ALL rendering related stuff is kept inside
  this one class. Avoid intertwingling at all costs! Otherwise we
  just get a huge mess.

  There are two basic rendering modes. The first is the 3D mode for
  rendering level graphics. The second is 2D mode for drawing HUD
  graphics, menus, the console etc.

  The renderer should work regardless of screen aspect ratio. This
  should make people with widescreen monitors happy.
*/
class OGLRenderer
{
  friend class spritepres_t;
  friend class ShaderProg;
private:
  bool  workinggl;  ///< Do we have a working OpenGL context?
  GLfloat glversion;  ///< Current (runtime) OpenGL version (major.minor).

  SDL_Surface *screen; ///< Main screen turn on.
  GLint viewportw; ///< Width of current viewport in pixels.
  GLint viewporth; ///< Height of current viewport in pixels.

  bool consolemode; ///< Are we drawing 3D level graphics or 2D console graphics.

  class Map *mp;  ///< Map to be rendered
  subsector_t *curssec; ///< The gl subsector the camera is in.
  GLfloat x, y, z; ///< Location of camera.
  GLfloat theta;   ///< Rotation angle of camera in degrees.
  GLfloat phi;     ///< Up-down rotation angle of camera in degrees.

  GLfloat fov;     ///< Field of view in degrees.

  fixed_t fr_cx, fr_cy; // Tip of current viewing frustum.
  fixed_t fr_lx, fr_ly; // Left edge of frustum.
  fixed_t fr_rx, fr_ry; // Right edge of frustum.

  GLfloat hudar;      ///< HUD aspect ratio.
  GLfloat screenar;   ///< Aspect ratio of the physical screen (monitor).
  GLfloat viewportar; ///< Aspect ratio of current viewport.

  RGB_t *palette;  ///< Converting palette data to OGL colors.

  void RenderBSPNode(int nodenum); ///< Render level using BSP.
  void RenderGLSubsector(int num);
  void RenderGlSsecPolygon(subsector_t *ss, GLfloat height, Material *tex, bool isFloor, GLfloat xoff=0.0, GLfloat yoff=0.0);
  void RenderGLSeg(int num);
  void GetSegQuads(int num, quad &u, quad &m, quad &l) const;
  void RenderActors(subsector_t *ssec);
  void DrawSingleQuad(Material *m, vertex_t *v1, vertex_t *v2, GLfloat lower, GLfloat upper, GLfloat texleft=0.0, GLfloat texright=1.0, GLfloat textop=0.0, GLfloat texbottom=1.0) const;
  void DrawSingleQuad(const quad *q) const;

  void DrawSimpleSky();

  void CalculateFrustum();
  bool BBoxIntersectsFrustum(const struct bbox_t& bbox) const; ///< True if bounding box intersects current view frustum.

public:
  OGLRenderer();
  ~OGLRenderer();
  bool InitVideoMode(const int w, const int h, const bool fullscreen);

  void InitGLState();
  void StartFrame();
  void FinishFrame();
  void ClearDrawColorAndLights();
  bool WriteScreenshot(const char *fname = NULL);

  bool ReadyToDraw() const { return workinggl; } // Console tries to draw to screen before video is initialized.

  bool In2DMode() const {return consolemode;}

  void SetGlobalColor(GLfloat *rgba);

  void SetFullScreenViewport();
  void SetViewport(unsigned vp);
  void Setup2DMode();
  void Draw2DGraphic(GLfloat left, GLfloat bottom, GLfloat right, GLfloat top, Material *mat,
		     GLfloat texleft=0.0, GLfloat texbottom=1.0, GLfloat texright=1.0, GLfloat textop=0.0);
  void Draw2DGraphic_Doom(GLfloat x, GLfloat y, Material *tex, int flags);
  void Draw2DGraphicFill_Doom(GLfloat x, GLfloat y, GLfloat width, GLfloat height, Material *tex);
  void ClearAutomap();
  void DrawAutomapLine(const fline_t *line, const int color);

  void Setup3DMode();

  void RenderPlayerView(PlayerInfo *player);
  void Render3DView(Actor *pov);
  void DrawPSprites(class PlayerPawn *p);

  enum spriteflag_t
  {
    BLEND_CONST = 0x00,
    BLEND_ADD   = 0x01,
    BLEND_MASK  = 0x03,
    FLIP_X      = 0x10,    
  };
  void DrawSpriteItem(const vec_t<fixed_t>& pos, Material *t, int flags, GLfloat alpha);

  bool CheckVis(int fromss, int toss);

  /// Draw the border around viewport.
  static void DrawViewBorder() {}
  static void DrawFill(int x, int y, int w, int h, int color) {}
  static void FadeScreenMenuBack(unsigned color, int height) {}
};

extern OGLRenderer *oglrenderer;

#endif
