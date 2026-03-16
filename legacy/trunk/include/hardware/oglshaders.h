// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2007 by DooM Legacy Team.
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
/// \brief GLSL shaders.

#ifndef oglshaders_h
#define oglshaders_h 1

/// Maximum number of dynamic point lights passed to GLSL shaders per frame.
#define MAX_SHADER_LIGHTS 16

// Define GLchar for MSYS2 MinGW compatibility
#ifndef GLchar
typedef char GLchar;
#endif

// GLEW provides GLSL function prototypes on Windows/MinGW
#if defined(_WIN32) || defined(__MINGW32__)
#define GLEW_STATIC
#include <GL/glew.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include "z_cache.h"

struct shader_attribs_t
{
    float tangent[3]; ///< surface tangent, points towards increasing s texture coord.
};

#if defined(GL_VERSION_2_0) && !defined(NO_SHADERS) // GLSL is introduced in OpenGL 2.0

/// GLSL shader object.
class Shader : public cacheitem_t
{
  protected:
    static const GLuint NOSHADER = 0;
    GLuint shader_id; ///< OpenGL handle for the shader.
    GLenum type;      ///< Vertex or fragment shader?
    bool ready;       ///< Compiled and functional?

    void Compile(const char *code, int len);

  public:
    Shader(const char *name, bool vertex_shader = true);
    Shader(const char *name, const char *code, bool vertex_shader = true);
    ~Shader();

    inline GLuint GetID() const
    {
        return shader_id;
    }
    inline bool IsReady() const
    {
        return ready;
    }

    void PrintInfoLog();
};

/// GLSL Program object.
class ShaderProg : public cacheitem_t
{
  protected:
    GLuint prog_id; ///< OpenGL handle for the program.
    /// Variable locations in the linked program.
    struct
    {
        GLint tex0, tex1; ///< Texture units 0-1 (diffuse, normal)
        GLint tex2, tex3, tex4, tex5, tex6, tex7; ///< PBR map units 2-7
        GLint time;
        GLint tangent;
        GLint normalmap_strength;
        GLint glossiness;    ///< Per-material specular shininess
        GLint specularlevel; ///< Per-material specular intensity
        GLint uCameraPos;    ///< Camera world position (for parallax)
        GLint uSpecularMaterial; ///< vec2(glossiness, specularlevel) for .fp shaders
        GLint uDynLightPos;   ///< base of vec4 uDynLightPos[MAX_SHADER_LIGHTS] (eye-space xyz + radius w)
        GLint uDynLightColor; ///< base of vec3 uDynLightColor[MAX_SHADER_LIGHTS]
        GLint uDynLightCount; ///< int uniform — number of active lights (<= MAX_SHADER_LIGHTS)
    } loc;

  public:
    ShaderProg(const char *name);
    ~ShaderProg();

    static void DisableShaders();

    GLuint GetID() const { return prog_id; }

    void AttachShader(Shader *s);
    void Link();
    void Use();
    void SetUniforms();
    void SetPerMaterialUniforms(float glossiness, float specularlevel); ///< Per-material scalars.
    void SetSamplerUniforms();   ///< Binds tex0-tex7 to units 0-7. Safe before map load.
    void SetFpSamplerUniforms(); ///< Binds GZDoom sampler names (normaltexture etc.) to units.
    void SetAttributes(shader_attribs_t *a);

    /// Upload dynamic point lights for this frame (eye-space positions + radii + colors).
    /// eyePositions4: array of count vec4s (xyz=eye-space pos, w=radius).
    /// colors3: array of count vec3s (rgb in [0,1]).
    void SetDynamicLights(const float *eyePositions4, const float *colors3, int count);

    void PrintInfoLog();
};

#else // GL_VERSION_2_0

// Inert dummy implementation
class Shader : public cacheitem_t
{
  public:
    Shader(const char *name, bool vertex_shader = true) : cacheitem_t(name)
    {
    }
};

// Inert dummy implementation
class ShaderProg : public cacheitem_t
{
  public:
    ShaderProg(const char *name) : cacheitem_t(name)
    {
    }
    static void DisableShaders()
    {
    }
    GLuint GetID() const { return 0; }
    void AttachShader(Shader *s) {};
    void Link() {};
    void Use()
    {
    }
    void SetUniforms()
    {
    }
    void SetSamplerUniforms()
    {
    }
    void SetAttributes(shader_attribs_t *a)
    {
    }
    void SetDynamicLights(const float *, const float *, int)
    {
    }
};

#endif // GL_VERSION_2_0

/// \brief Cache for Shaders
class shader_cache_t : public cache_t<Shader>
{
  protected:
    virtual Shader *Load(const char *name)
    {
        return NULL;
    };
};

extern shader_cache_t shaders;

/// \brief Cache for ShaderProgs
class shaderprog_cache_t : public cache_t<ShaderProg>
{
  protected:
    virtual ShaderProg *Load(const char *name)
    {
        return new ShaderProg(name);
    };
};

extern shaderprog_cache_t shaderprogs;

/// Compiled normal-map shader program. NULL until InitNormalMapShader() succeeds.
extern ShaderProg *normalmap_shaderprog;

/// Compile and link the normal-map GLSL program and create neutral PBR textures.
/// Must be called after an OpenGL context exists (i.e., from R_Init or later).
/// Safe to call multiple times; subsequent calls are no-ops.
void InitNormalMapShader();

/// Delete neutral PBR textures (call before GL context is destroyed).
void DeletePBRNeutralTextures();

/// Bind a 1x1 neutral texture to the given texture unit for an unused PBR slot.
/// slot 1=normal(flat), 2=metallic(black), 3=roughness(gray), 4=ao(white),
/// 5=specular(black), 6=brightmap(black), 7=displacement(gray).
void BindPBRNeutral(int slot);

/// Compile (or retrieve from cache) a ShaderProg wrapping a GZDoom .fp snippet.
/// fp_lump_path: full path in the file cache (e.g. "shaders/displacement.fp").
/// Returns NULL on failure; on success the program is ready to use.
ShaderProg *CompileFpShader(const char *fp_lump_path);

/// For each material whose shader_lump is non-empty, compile and assign its .fp shader.
/// Falls back to normalmap_shaderprog on failure.
/// Must be called after InitNormalMapShader() and after the GL context is ready.
void CompileGLDEFSShaders();

/// Delete all compiled .fp shader objects (call before GL context is destroyed).
void ClearFpShaders();

#endif
