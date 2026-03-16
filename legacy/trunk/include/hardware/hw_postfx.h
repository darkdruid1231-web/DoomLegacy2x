// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2024 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Post-processing effects: bloom and SSAO.

#ifndef hw_postfx_h
#define hw_postfx_h 1

#if defined(_WIN32) || defined(__MINGW32__)
#define GLEW_STATIC
#include <GL/glew.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#endif

class ShaderProg;

/// Post-processing pipeline: bloom and SSAO.
/// Renders the scene to an FBO, then composites effects before swap.
class PostFX
{
public:
    PostFX();
    ~PostFX();

    /// Allocate FBOs and compile shaders. Call after GL context is ready.
    /// Returns false if FBOs are not supported (effects silently disabled).
    bool Init(int w, int h);

    /// Release all GL resources.
    void Destroy();

    /// Reallocate FBOs when the window is resized.
    void Resize(int w, int h);

    /// Bind the scene FBO so subsequent rendering goes into it.
    /// Call at the start of each 3D frame when IsActive().
    void BindSceneFBO();

    /// Unbind the scene FBO and run all enabled effects, compositing to the
    /// default framebuffer. Call just before SDL_GL_SwapWindow.
    void ApplyEffects();

    /// True when at least one effect is enabled and FBOs are ready.
    bool IsActive() const;

private:
    bool fbo_ready;         ///< Init succeeded (FBOs allocated).

    // Scene FBO (full resolution)
    GLuint scene_fbo;
    GLuint scene_color_tex;
    GLuint scene_depth_tex;

    // Bloom ping-pong FBOs (half resolution)
    GLuint bloom_fbo[2];
    GLuint bloom_tex[2];

    // SSAO FBOs
    GLuint ssao_fbo;
    GLuint ssao_tex;
    GLuint ssao_blur_fbo;
    GLuint ssao_blur_tex;

    // Post-processing shaders
    ShaderProg *prog_brightpass;
    ShaderProg *prog_blur_h;
    ShaderProg *prog_blur_v;
    ShaderProg *prog_composite;
    ShaderProg *prog_ssao;
    ShaderProg *prog_ssao_blur;
    ShaderProg *prog_ssao_composite;

    int width, height;

    float ssao_kernel[16][3]; ///< 16-sample hemisphere kernel (view-space tangent-space).

    void BuildSSAOKernel();

    /// Allocate one 2D FBO with a single color texture attachment.
    /// Returns false on GL error. fbo and tex receive the allocated names.
    bool CreateColorFBO(GLuint &fbo, GLuint &tex, int w, int h, GLenum fmt = GL_RGBA8);

    /// Draw a unit NDC quad covering [-1,1] in X and Y.
    void RenderFullscreenQuad();

    void BloomPass();
    void SSAOPass();
    void CompileShaders();
    void DeleteShaders();
};

extern PostFX postfx;

#endif // hw_postfx_h
