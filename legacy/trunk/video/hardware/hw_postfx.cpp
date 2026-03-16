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
/// \brief Post-processing effects: bloom and screen-space ambient occlusion.

#define GL_GLEXT_PROTOTYPES 1

#if defined(_WIN32) || defined(__MINGW32__)
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#include "hardware/hw_postfx.h"
#include "hardware/oglshaders.h"
#include "command.h"   // full consvar_t definition
#include "cvars.h"
#include "console.h"
#include <cmath>
#include <cstring>
#include <cstdlib>

PostFX postfx;

// ---------------------------------------------------------------------------
// Simple vertex shader reused by all fullscreen-quad passes.
// gl_MultiTexCoord0.xy is 0..1; vertex position is derived from it.
// ---------------------------------------------------------------------------
static const char *postfx_vert_src =
    "#version 130\n"
    "out vec2 vTexCoord;\n"
    "void main() {\n"
    "    vTexCoord = gl_MultiTexCoord0.xy;\n"
    "    gl_Position = vec4(gl_MultiTexCoord0.xy * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Bloom: bright-pass
// ---------------------------------------------------------------------------
static const char *bloom_brightpass_frag_src =
    "#version 130\n"
    "uniform sampler2D uScene;\n"
    "uniform float uThreshold;\n"
    "uniform float uStrength;\n"
    "in vec2 vTexCoord;\n"
    "void main() {\n"
    "    vec3 c = texture(uScene, vTexCoord).rgb;\n"
    "    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    "    float knee = 0.1;\n"
    "    float w = smoothstep(uThreshold - knee, uThreshold + knee, lum);\n"
    "    gl_FragColor = vec4(c * w * uStrength, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Bloom: separable Gaussian blur (7-tap, bilinear-optimised)
// uDir should be (1/texWidth, 0) for horizontal, (0, 1/texHeight) for vertical.
// ---------------------------------------------------------------------------
static const char *bloom_blur_frag_src =
    "#version 130\n"
    "uniform sampler2D uBloom;\n"
    "uniform vec2 uDir;\n"
    "in vec2 vTexCoord;\n"
    "void main() {\n"
    "    vec2 off1 = 1.411764706 * uDir;\n"
    "    vec2 off2 = 3.294117647 * uDir;\n"
    "    vec2 off3 = 5.176470588 * uDir;\n"
    "    vec3 c = texture(uBloom, vTexCoord         ).rgb * 0.1964825502;\n"
    "    c += texture(uBloom, vTexCoord + off1).rgb * 0.2969069646;\n"
    "    c += texture(uBloom, vTexCoord - off1).rgb * 0.2969069646;\n"
    "    c += texture(uBloom, vTexCoord + off2).rgb * 0.0944703983;\n"
    "    c += texture(uBloom, vTexCoord - off2).rgb * 0.0944703983;\n"
    "    c += texture(uBloom, vTexCoord + off3).rgb * 0.0103813938;\n"
    "    c += texture(uBloom, vTexCoord - off3).rgb * 0.0103813938;\n"
    "    gl_FragColor = vec4(c, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Bloom: composite scene + bloom
// ---------------------------------------------------------------------------
static const char *bloom_composite_frag_src =
    "#version 130\n"
    "uniform sampler2D uScene;\n"
    "uniform sampler2D uBloom;\n"
    "uniform float uBloomIntensity;\n"
    "in vec2 vTexCoord;\n"
    "void main() {\n"
    "    vec3 scene = texture(uScene, vTexCoord).rgb;\n"
    "    vec3 bloom = texture(uBloom, vTexCoord).rgb;\n"
    "    gl_FragColor = vec4(scene + bloom * uBloomIntensity, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// SSAO: reconstruct view-space position from depth, sample hemisphere
// ---------------------------------------------------------------------------
static const char *ssao_frag_src =
    "#version 130\n"
    "uniform sampler2D uDepth;\n"
    "uniform mat4 uProjInv;\n"
    "uniform vec3 uKernel[16];\n"
    "uniform float uRadius;\n"
    "uniform float uBias;\n"
    "in vec2 vTexCoord;\n"
    "\n"
    "vec3 ReconstructPos(vec2 uv) {\n"
    "    float d = texture(uDepth, uv).r;\n"
    "    vec4 clip = vec4(uv * 2.0 - 1.0, d * 2.0 - 1.0, 1.0);\n"
    "    vec4 view = uProjInv * clip;\n"
    "    return view.xyz / view.w;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec3 fragPos = ReconstructPos(vTexCoord);\n"
    "    // Reconstruct view-space normal from position screen-space derivatives\n"
    "    vec3 n1 = dFdx(fragPos);\n"
    "    vec3 n2 = dFdy(fragPos);\n"
    "    vec3 normal = normalize(cross(n1, n2));\n"
    "    if (dot(normal, -fragPos) < 0.0) normal = -normal;\n"
    "\n"
    "    // Per-pixel rotation using a screen-space hash to reduce banding\n"
    "    float angle = fract(sin(dot(vTexCoord, vec2(12.9898, 78.233))) * 43758.5453) * 6.2832;\n"
    "    float cosA = cos(angle), sinA = sin(angle);\n"
    "    vec3 rvec = vec3(cosA, sinA, 0.0);\n"
    "    vec3 tangent   = normalize(rvec - normal * dot(rvec, normal));\n"
    "    vec3 bitangent = cross(normal, tangent);\n"
    "    mat3 TBN = mat3(tangent, bitangent, normal);\n"
    "\n"
    "    float occlusion = 0.0;\n"
    "    for (int i = 0; i < 16; i++) {\n"
    "        vec3 sampleVS = fragPos + TBN * uKernel[i] * uRadius;\n"
    "        // Approximate screen UV for sample\n"
    "        vec2 sampleUV = vTexCoord + sampleVS.xy /\n"
    "                        (max(abs(fragPos.z), 0.1)) * 0.5;\n"
    "        sampleUV = clamp(sampleUV, 0.001, 0.999);\n"
    "        vec3 sampleActual = ReconstructPos(sampleUV);\n"
    "        float rangeCheck = smoothstep(0.0, 1.0,\n"
    "                           uRadius / max(abs(fragPos.z - sampleActual.z), 0.001));\n"
    "        occlusion += (sampleActual.z >= sampleVS.z + uBias ? 1.0 : 0.0) * rangeCheck;\n"
    "    }\n"
    "    float ao = 1.0 - (occlusion / 16.0);\n"
    "    gl_FragColor = vec4(ao, ao, ao, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// SSAO blur: 3x3 box
// ---------------------------------------------------------------------------
static const char *ssao_blur_frag_src =
    "#version 130\n"
    "uniform sampler2D uSSAOTex;\n"
    "uniform vec2 uTexelSize;\n"
    "in vec2 vTexCoord;\n"
    "void main() {\n"
    "    float result = 0.0;\n"
    "    for (int x = -1; x <= 1; x++) {\n"
    "        for (int y = -1; y <= 1; y++) {\n"
    "            vec2 off = vec2(float(x), float(y)) * uTexelSize;\n"
    "            result += texture(uSSAOTex, vTexCoord + off).r;\n"
    "        }\n"
    "    }\n"
    "    result /= 9.0;\n"
    "    gl_FragColor = vec4(result, result, result, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// SSAO composite: multiply scene color by AO factor
// ---------------------------------------------------------------------------
static const char *ssao_composite_frag_src =
    "#version 130\n"
    "uniform sampler2D uScene;\n"
    "uniform sampler2D uSSAOBlur;\n"
    "uniform float uSSAOStrength;\n"
    "in vec2 vTexCoord;\n"
    "void main() {\n"
    "    vec3 scene = texture(uScene, vTexCoord).rgb;\n"
    "    float ao = texture(uSSAOBlur, vTexCoord).r;\n"
    "    float factor = mix(1.0, ao, uSSAOStrength);\n"
    "    gl_FragColor = vec4(scene * factor, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Helper: compile a vertex+fragment pair and link.  Returns NULL on failure.
static ShaderProg *MakePostFXProg(const char *name, const char *frag_src)
{
    Shader *vs = new Shader(name, postfx_vert_src, true);
    Shader *fs = new Shader(name, frag_src,        false);
    if (!vs->IsReady() || !fs->IsReady())
    {
        CONS_Printf("PostFX: shader '%s' failed to compile.\n", name);
        delete vs; delete fs;
        return NULL;
    }
    ShaderProg *prog = new ShaderProg(name);
    prog->AttachShader(vs);
    prog->AttachShader(fs);
    prog->Link();
    return prog;
}

/// Get uniform location from a ShaderProg (convenience wrapper).
static inline GLint U(ShaderProg *p, const char *name)
{
    return glGetUniformLocation(p->GetID(), name);
}

// ---------------------------------------------------------------------------
// PostFX implementation
// ---------------------------------------------------------------------------

PostFX::PostFX()
    : fbo_ready(false)
    , scene_fbo(0), scene_color_tex(0), scene_depth_tex(0)
    , ssao_fbo(0), ssao_tex(0), ssao_blur_fbo(0), ssao_blur_tex(0)
    , prog_brightpass(NULL), prog_blur_h(NULL), prog_blur_v(NULL)
    , prog_composite(NULL), prog_ssao(NULL), prog_ssao_blur(NULL)
    , prog_ssao_composite(NULL)
    , width(0), height(0)
{
    bloom_fbo[0] = bloom_fbo[1] = 0;
    bloom_tex[0] = bloom_tex[1] = 0;
    memset(ssao_kernel, 0, sizeof(ssao_kernel));
}

PostFX::~PostFX()
{
    Destroy();
}

void PostFX::CompileShaders()
{
    prog_brightpass     = MakePostFXProg("postfx_brightpass",     bloom_brightpass_frag_src);
    prog_blur_h         = MakePostFXProg("postfx_blur_h",         bloom_blur_frag_src);
    prog_blur_v         = MakePostFXProg("postfx_blur_v",         bloom_blur_frag_src);
    prog_composite      = MakePostFXProg("postfx_composite",      bloom_composite_frag_src);
    prog_ssao           = MakePostFXProg("postfx_ssao",           ssao_frag_src);
    prog_ssao_blur      = MakePostFXProg("postfx_ssao_blur",      ssao_blur_frag_src);
    prog_ssao_composite = MakePostFXProg("postfx_ssao_composite", ssao_composite_frag_src);
}

void PostFX::DeleteShaders()
{
    delete prog_brightpass;     prog_brightpass     = NULL;
    delete prog_blur_h;         prog_blur_h         = NULL;
    delete prog_blur_v;         prog_blur_v         = NULL;
    delete prog_composite;      prog_composite      = NULL;
    delete prog_ssao;           prog_ssao           = NULL;
    delete prog_ssao_blur;      prog_ssao_blur      = NULL;
    delete prog_ssao_composite; prog_ssao_composite = NULL;
}

bool PostFX::CreateColorFBO(GLuint &fbo, GLuint &tex, int w, int h, GLenum fmt)
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

void PostFX::BuildSSAOKernel()
{
    // 16-sample hemisphere in tangent space (z >= 0).
    // Fixed seed for deterministic samples.
    srand(12345);
    for (int i = 0; i < 16; i++)
    {
        float x = (float)(rand() % 2001 - 1000) / 1000.0f;
        float y = (float)(rand() % 2001 - 1000) / 1000.0f;
        float z = (float)(rand() % 1001)         / 1000.0f;
        float len = sqrtf(x*x + y*y + z*z);
        if (len < 1e-4f) { x = 0; y = 0; z = 1; len = 1.0f; }
        x /= len; y /= len; z /= len;
        // Accelerating interpolation: weight closer samples more
        float scale = (float)i / 16.0f;
        scale = 0.1f + 0.9f * scale * scale;
        ssao_kernel[i][0] = x * scale;
        ssao_kernel[i][1] = y * scale;
        ssao_kernel[i][2] = z * scale;
    }
    srand(0);
}

bool PostFX::Init(int w, int h)
{
#if defined(GL_VERSION_2_0) && !defined(NO_SHADERS)
    // Check for FBO support
    if (!GLEW_VERSION_3_0 && !GLEW_ARB_framebuffer_object)
    {
        CONS_Printf("PostFX: FBOs not supported, post-processing disabled.\n");
        return false;
    }
#else
    return false;
#endif

    width = w; height = h;

    // --- Scene FBO (full resolution + depth) ---
    glGenFramebuffers(1, &scene_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, scene_fbo);

    glGenTextures(1, &scene_color_tex);
    glBindTexture(GL_TEXTURE_2D, scene_color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, scene_color_tex, 0);

    glGenTextures(1, &scene_depth_tex);
    glBindTexture(GL_TEXTURE_2D, scene_depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, scene_depth_tex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        CONS_Printf("PostFX: scene FBO incomplete (0x%x), disabling.\n", status);
        Destroy();
        return false;
    }

    // --- Bloom ping-pong FBOs at half resolution ---
    int bw = w / 2, bh = h / 2;
    if (bw < 1) bw = 1;
    if (bh < 1) bh = 1;
    if (!CreateColorFBO(bloom_fbo[0], bloom_tex[0], bw, bh) ||
        !CreateColorFBO(bloom_fbo[1], bloom_tex[1], bw, bh))
    {
        CONS_Printf("PostFX: bloom FBOs failed, disabling.\n");
        Destroy();
        return false;
    }

    // --- SSAO FBOs (full resolution, single channel) ---
    // Use GL_RED / GL_R8 if available, fall back to GL_RGBA8
    GLenum ssao_fmt = GL_R8;
    if (!CreateColorFBO(ssao_fbo, ssao_tex, w, h, ssao_fmt))
    {
        ssao_fmt = GL_RGBA8;
        CreateColorFBO(ssao_fbo, ssao_tex, w, h, ssao_fmt);
    }
    if (!CreateColorFBO(ssao_blur_fbo, ssao_blur_tex, w, h, ssao_fmt))
    {
        CONS_Printf("PostFX: SSAO FBOs failed, SSAO disabled.\n");
        ssao_fbo = ssao_tex = ssao_blur_fbo = ssao_blur_tex = 0;
    }

    CompileShaders();
    BuildSSAOKernel();

    fbo_ready = true;
    CONS_Printf("PostFX: initialized %dx%d.\n", w, h);
    return true;
}

void PostFX::Destroy()
{
    DeleteShaders();

    if (scene_fbo)       { glDeleteFramebuffers(1, &scene_fbo);   scene_fbo       = 0; }
    if (scene_color_tex) { glDeleteTextures(1, &scene_color_tex);  scene_color_tex = 0; }
    if (scene_depth_tex) { glDeleteTextures(1, &scene_depth_tex);  scene_depth_tex = 0; }

    for (int i = 0; i < 2; i++)
    {
        if (bloom_fbo[i]) { glDeleteFramebuffers(1, &bloom_fbo[i]); bloom_fbo[i] = 0; }
        if (bloom_tex[i]) { glDeleteTextures(1,    &bloom_tex[i]);  bloom_tex[i] = 0; }
    }

    if (ssao_fbo)      { glDeleteFramebuffers(1, &ssao_fbo);      ssao_fbo      = 0; }
    if (ssao_tex)      { glDeleteTextures(1,     &ssao_tex);       ssao_tex      = 0; }
    if (ssao_blur_fbo) { glDeleteFramebuffers(1, &ssao_blur_fbo);  ssao_blur_fbo = 0; }
    if (ssao_blur_tex) { glDeleteTextures(1,     &ssao_blur_tex);  ssao_blur_tex = 0; }

    fbo_ready = false;
}

void PostFX::Resize(int w, int h)
{
    Destroy();
    Init(w, h);
}

bool PostFX::IsActive() const
{
    if (!fbo_ready) return false;
    return cv_grbloom.value || cv_grssao.value;
}

void PostFX::BindSceneFBO()
{
    if (fbo_ready && scene_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, scene_fbo);
}

void PostFX::RenderFullscreenQuad()
{
    // Vertex positions are derived from the texcoord in the vertex shader.
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();
}

void PostFX::SSAOPass()
{
    if (!ssao_fbo || !prog_ssao || !prog_ssao_blur || !prog_ssao_composite)
        return;

    // Compute inverse projection for depth → view-space reconstruction.
    GLfloat proj[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    GLfloat pi[16];
    memset(pi, 0, sizeof(pi));
    // Standard GL perspective inverse (column-major).
    // See derivation: P^-1 columns:
    //   col 0: [1/m[0], 0, 0, 0]
    //   col 1: [0, 1/m[5], 0, 0]
    //   col 2: [0, 0, 0, 1/m[14]]
    //   col 3: [0, 0, -1, m[10]/m[14]]
    if (fabsf(proj[0])  > 1e-7f) pi[0]  = 1.0f / proj[0];
    if (fabsf(proj[5])  > 1e-7f) pi[5]  = 1.0f / proj[5];
    if (fabsf(proj[14]) > 1e-7f) pi[11] = 1.0f / proj[14];
    pi[14] = proj[11];                                           // = -1
    if (fabsf(proj[14]) > 1e-7f) pi[15] = proj[10] / proj[14]; // A/B

    // --- SSAO pass → ssao_tex ---
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    prog_ssao->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_depth_tex);
    GLint l = U(prog_ssao, "uDepth");     if (l >= 0) glUniform1i(l, 0);
    l = U(prog_ssao, "uProjInv");         if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, pi);
    l = U(prog_ssao, "uKernel");          if (l >= 0) glUniform3fv(l, 16, &ssao_kernel[0][0]);
    l = U(prog_ssao, "uRadius");          if (l >= 0) glUniform1f(l, 64.0f);
    l = U(prog_ssao, "uBias");            if (l >= 0) glUniform1f(l, 0.025f);
    RenderFullscreenQuad();

    // --- Blur → ssao_blur_tex ---
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_fbo);
    glClear(GL_COLOR_BUFFER_BIT);

    prog_ssao_blur->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssao_tex);
    l = U(prog_ssao_blur, "uSSAOTex");    if (l >= 0) glUniform1i(l, 0);
    l = U(prog_ssao_blur, "uTexelSize");  if (l >= 0) glUniform2f(l, 1.0f / width, 1.0f / height);
    RenderFullscreenQuad();

    // --- Composite SSAO into scene → bloom_fbo[0] (used as scratch full-res buffer) ---
    // We borrow bloom_fbo[0] at full resolution temporarily.
    // The bloom FBO is half-res but CreateColorFBO allocated it at bw x bh;
    // we rebind its texture to a new full-res FBO if needed. Instead, we use
    // a separate scratch: ssao_tex (which we just consumed) is now free.
    // Render composite into ssao_tex (reusing ssao_fbo with ssao_tex already attached).
    // Output replaces ssao_tex content. Bloom pass will pick the right source.
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);  // write back to ssao_tex
    glClear(GL_COLOR_BUFFER_BIT);

    prog_ssao_composite->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_color_tex);
    l = U(prog_ssao_composite, "uScene");       if (l >= 0) glUniform1i(l, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ssao_blur_tex);
    l = U(prog_ssao_composite, "uSSAOBlur");    if (l >= 0) glUniform1i(l, 1);
    l = U(prog_ssao_composite, "uSSAOStrength"); if (l >= 0) glUniform1f(l, cv_grssaostrength.value);
    RenderFullscreenQuad();

    ShaderProg::DisableShaders();
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}

void PostFX::BloomPass()
{
    if (!prog_brightpass || !prog_blur_h || !prog_blur_v || !prog_composite)
        return;

    int bw = width / 2, bh = height / 2;
    if (bw < 1) bw = 1;
    if (bh < 1) bh = 1;

    // Source: SSAO-composited scene if SSAO ran, otherwise raw scene.
    GLuint src = (cv_grssao.value && ssao_fbo && ssao_tex) ? ssao_tex : scene_color_tex;

    // --- Bright-pass → bloom_fbo[0] (half res) ---
    glBindFramebuffer(GL_FRAMEBUFFER, bloom_fbo[0]);
    glViewport(0, 0, bw, bh);
    glClear(GL_COLOR_BUFFER_BIT);

    prog_brightpass->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src);
    GLint l = U(prog_brightpass, "uScene");     if (l >= 0) glUniform1i(l, 0);
    l = U(prog_brightpass, "uThreshold");       if (l >= 0) glUniform1f(l, cv_grbloomthreshold.value);
    l = U(prog_brightpass, "uStrength");        if (l >= 0) glUniform1f(l, cv_grbloomstrength.value);
    RenderFullscreenQuad();

    // --- Horizontal blur → bloom_fbo[1] ---
    glBindFramebuffer(GL_FRAMEBUFFER, bloom_fbo[1]);
    glClear(GL_COLOR_BUFFER_BIT);
    prog_blur_h->Use();
    glBindTexture(GL_TEXTURE_2D, bloom_tex[0]);
    l = U(prog_blur_h, "uBloom"); if (l >= 0) glUniform1i(l, 0);
    l = U(prog_blur_h, "uDir");   if (l >= 0) glUniform2f(l, 1.0f / bw, 0.0f);
    RenderFullscreenQuad();

    // --- Vertical blur → bloom_fbo[0] ---
    glBindFramebuffer(GL_FRAMEBUFFER, bloom_fbo[0]);
    glClear(GL_COLOR_BUFFER_BIT);
    prog_blur_v->Use();
    glBindTexture(GL_TEXTURE_2D, bloom_tex[1]);
    l = U(prog_blur_v, "uBloom"); if (l >= 0) glUniform1i(l, 0);
    l = U(prog_blur_v, "uDir");   if (l >= 0) glUniform2f(l, 0.0f, 1.0f / bh);
    RenderFullscreenQuad();

    // --- Composite scene + bloom → default framebuffer ---
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    prog_composite->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src);
    l = U(prog_composite, "uScene");          if (l >= 0) glUniform1i(l, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloom_tex[0]);
    l = U(prog_composite, "uBloom");          if (l >= 0) glUniform1i(l, 1);
    l = U(prog_composite, "uBloomIntensity"); if (l >= 0) glUniform1f(l, 1.0f);
    RenderFullscreenQuad();

    ShaderProg::DisableShaders();
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}

void PostFX::ApplyEffects()
{
    if (!fbo_ready) return;

    // Unbind scene FBO — subsequent rendering goes to the default framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Save relevant GL state.
    glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_FOG);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // --- SSAO (must run before bloom so bloom picks up the AO-darkened scene) ---
    if (cv_grssao.value && ssao_fbo)
        SSAOPass();

    // --- Bloom ---
    if (cv_grbloom.value)
    {
        BloomPass();
    }
    else
    {
        // No bloom — blit the (possibly SSAO-composited) scene directly to screen.
        GLuint src = (cv_grssao.value && ssao_fbo && ssao_tex)
                     ? ssao_tex : scene_color_tex;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        ShaderProg::DisableShaders();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, src);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2f(-1,-1);
        glTexCoord2f(1,0); glVertex2f( 1,-1);
        glTexCoord2f(1,1); glVertex2f( 1, 1);
        glTexCoord2f(0,1); glVertex2f(-1, 1);
        glEnd();
    }

    ShaderProg::DisableShaders();
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glPopAttrib();
}
