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
#ifdef SDL2
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
#ifdef USE_SDL2
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

#include<GL/gl.h>
#include<GL/glext.h>

// On Windows without GLEW, <GL/gl.h> only covers OpenGL 1.1. Declare the 1.4
// point parameter functions explicitly. When GLEW is used, it provides these
// via function pointer macros so this block must be skipped.
#if defined(_WIN32) && !defined(GL_POINT_PARAMETERS_DECLARED) && !defined(__GLEW_H__)
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

struct quadtexcoord {
  GLfloat l;
  GLfloat r;
  GLfloat t;
  GLfloat b;
};

struct quad {
  Material *m;
  struct quadtexcoord t;
  vertex_t *v1;
  vertex_t *v2;
  GLfloat bottom;
  GLfloat top;
  int lightlevel;  // Sector light level for this quad
  float cmr, cmg, cmb;  // Sector colormap tint multipliers (1.0 = no tint)
};

/// A single dynamic light valid for one rendered frame.
struct FrameLight {
  float x, y, z;        ///< World position
  float r, g, b;        ///< Light color (0-1)
  float radius;         ///< Attenuation radius in world units
  bool  isProjectile;   ///< True for missile/explosion sprites — enables corona flicker
};

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

#ifdef SDL2
  SDL_Window *screen;    ///< Main screen window (SDL2)
  SDL_GLContext glCtx;   ///< OpenGL context (SDL2)
#else
  SDL_Surface *screen;   ///< Main screen surface (SDL1)
#endif
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

  // Dynamic lights collected at the start of each 3D frame.
  std::vector<FrameLight> framelights;
  void CollectDynamicLights(class Actor *pov);
  void AccumDynLight(float px, float py, float pz, float &r, float &g, float &b) const;
  /// Transform framelights to eye-space and upload as shader uniforms.
  void SetShaderDynamicLights(class ShaderProg *prog) const;

  // Blob shadow system.
  GLuint shadowTex;         ///< Circular gradient texture for blob shadows.
  void InitShadowTexture();

  // Corona system.
  GLuint coronaTex;         ///< Wide Gaussian glow texture for corona halos.
  void InitCoronaTexture();

  // Matrix UBO (GL 3.1+) — eliminates per-surface glGetFloatv stalls
  GLuint matrix_ubo;
  GLfloat cached_mv[16];    ///< Cached modelview matrix (column-major)
  GLfloat cached_proj[16];  ///< Cached projection matrix (column-major)

  // Shadow map state (Phase 2.3)
  bool  shadow_active;          ///< True when a shadow caster was found this frame.
  int   shadow_caster_idx;      ///< Index into framelights[] of the shadow caster (-1 = none).

  // Per-subsector fog parameters cached for shader use
  GLfloat fog_color[3];
  GLfloat fog_start, fog_end;

  // Fog state caching to skip redundant glEnable/Disable per subsector
  GLboolean last_fog_enabled;
  int last_fog_lightlevel;
  GLfloat last_fog_start, last_fog_end;

  // Texture bind tracking to skip redundant GLUse() calls
  mutable Material *current_material;

  void UploadMatrixUBO();
  void DrawBlobShadow(class Actor *thing) const;
  void DrawAllBlobShadows() const;
  void DrawAllCoronas() const;  ///< Draw additive corona halos at dynamic light positions.

  void RenderBSPNode(int nodenum); ///< Render level using BSP.
  void RenderGLSubsector(int num);
  void RenderShadowBSPNode(int nodenum);
  void RenderShadowSubsector(int num);
  void RenderGlSsecPolygon(subsector_t *ss, GLfloat height, Material *tex, bool isFloor, GLfloat xoff=0.0, GLfloat yoff=0.0, int lightlevel=255);
  void RenderGLSeg(int num);
  void GetSegQuads(int num, quad &u, quad &m, quad &l) const;
  void DrawSingleQuad(Material *m, vertex_t *v1, vertex_t *v2, GLfloat lower, GLfloat upper, GLfloat texleft=0.0, GLfloat texright=1.0, GLfloat textop=0.0, GLfloat texbottom=1.0, int lightlevel=255, float cmr=1.0f, float cmg=1.0f, float cmb=1.0f) const;
  void DrawSingleQuad(const quad *q) const;

  void DrawSimpleSky();

  void CalculateFrustum();
  bool BBoxIntersectsFrustum(const struct bbox_t& bbox) const; ///< True if bounding box intersects current view frustum.

public:
  OGLRenderer();
  ~OGLRenderer();
  bool InitVideoMode(const int w, const int h, const int displaymode);

  void InitGLState();
  void StartFrame();
  void FinishFrame();
  void ClearDrawColorAndLights();
  bool WriteScreenshot(const char *fname = NULL);

  // Profiling counters (reset each frame in StartFrame)
  mutable int frame_wall_quads;
  mutable int frame_flushes;
  mutable int frame_texture_binds;
  mutable int frame_sprite_draws;
  mutable int frame_sprite_batches;
  // Sprite state tracking for batching
  Material *last_sprite_mat;
  float last_sprite_alpha;
  int last_sprite_flags;
  // Helper to increment profiling counters from const methods
  void inc_profiling_counters(bool texture_bind);

  bool ReadyToDraw() const { return workinggl; } // Console tries to draw to screen before video is initialized.

#ifdef SDL2
  SDL_Window *GetScreen() const { return screen; }
#endif

  bool In2DMode() const {return consolemode;}

  void SetGlobalColor(GLfloat *rgba);

  void SetFullScreenViewport();
  void SetViewport(unsigned vp);
  void Setup2DMode();
  void Setup2DMode_Full(); ///< Like Setup2DMode but without HUD aspect-ratio pillarboxing (full screen width).
  void Draw2DGraphic(GLfloat left, GLfloat bottom, GLfloat right, GLfloat top, Material *mat,
		     GLfloat texleft=0.0, GLfloat texbottom=1.0, GLfloat texright=1.0, GLfloat textop=0.0);
  void Draw2DGraphic_Doom(GLfloat x, GLfloat y, Material *tex, int flags);
  void Draw2DGraphicFill_Doom(GLfloat x, GLfloat y, GLfloat width, GLfloat height, Material *tex);
  /// Draw a graphic stretched to the full screen, bypassing the HUD aspect-ratio transform.
  void DrawFullscreenGraphic(Material *mat, float worldwidth, float worldheight);
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

  /// Draw full-screen semi-transparent console background, bypassing HUD aspect-ratio centering.
  /// height_frac: fraction of screen height covered (0.0 = none, 1.0 = full screen).
  /// alpha: opacity 0.0 (invisible) to 1.0 (opaque).
  void DrawConsoleBackground(float height_frac, float alpha);
};

extern OGLRenderer *oglrenderer;

#endif
