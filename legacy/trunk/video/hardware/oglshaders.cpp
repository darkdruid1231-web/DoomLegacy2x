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
/// \brief GLSL shader implementation.

#include "hardware/oglshaders.h"

shader_cache_t shaders;
shaderprog_cache_t shaderprogs;
ShaderProg *normalmap_shaderprog = NULL;

#ifdef NO_SHADERS
#warning Shaders not included in the build.
#elif !defined(GL_VERSION_2_0) // GLSL is introduced in OpenGL 2.0
#warning Shaders require OpenGL 2.0!
#else

#include "command.h"
#include "cvars.h"
#include "doomdef.h"
#include "g_map.h"
#include "hardware/oglrenderer.hpp"
#include "r_data.h"
#include "w_wad.h"
#include "z_zone.h"
#include <GL/glu.h>
#include <map>
#include <string>

static void PrintError()
{
    GLenum err = glGetError();
    while (err != GL_NO_ERROR)
    {
        CONS_Printf("OpenGL error: %s\n", gluErrorString(err));
        err = glGetError();
    }
}

Shader::Shader(const char *n, bool vertex_shader)
    : cacheitem_t(n), shader_id(NOSHADER), ready(false)
{
    type = vertex_shader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;

    int lump = fc.FindNumForName(name);
    if (lump < 0)
        return;

    int len = fc.LumpLength(lump);
    char *code = static_cast<char *>(fc.CacheLumpNum(lump, PU_DAVE));
    Compile(code, len);
    Z_Free(code);
}

Shader::Shader(const char *n, const char *code, bool vertex_shader)
    : cacheitem_t(n), shader_id(NOSHADER), ready(false)
{
    type = vertex_shader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    Compile(code, strlen(code));
}

Shader::~Shader()
{
    if (shader_id != NOSHADER)
        glDeleteShader(shader_id);
}

void Shader::Compile(const char *code, int len)
{
    shader_id = glCreateShader(type);
    PrintError();
    glShaderSource(shader_id, 1, &code, &len);
    PrintError();
    glCompileShader(shader_id);
    PrintError();

    PrintInfoLog();

    GLint status = 0;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
    ready = (status == GL_TRUE);
    if (!ready)
        CONS_Printf("Shader '%s' would not compile.\n", name);
}

void Shader::PrintInfoLog()
{
    int len = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &len);

    if (len > 0)
    {
        char *log = static_cast<char *>(Z_Malloc(len, PU_DAVE, NULL));
        int chars = 0;
        glGetShaderInfoLog(shader_id, len, &chars, log);
        CONS_Printf("Shader '%s' InfoLog: %s\n", name, log);
        Z_Free(log);
    }
}

ShaderProg::ShaderProg(const char *n) : cacheitem_t(n)
{
    prog_id = glCreateProgram();
    PrintError();
}

ShaderProg::~ShaderProg()
{
    glDeleteProgram(prog_id);
}

void ShaderProg::DisableShaders()
{
    glUseProgram(0);
}

void ShaderProg::AttachShader(Shader *s)
{
    glAttachShader(prog_id, s->GetID());
    PrintError();
}

void ShaderProg::Link()
{
    glLinkProgram(prog_id);
    PrintError();

    GLint status = 0;
    glGetProgramiv(prog_id, GL_LINK_STATUS, &status);
    PrintError();
    if (status == GL_FALSE)
        CONS_Printf("Shader program '%s' could not be linked.\n", name);

    struct shader_var_t
    {
        GLint *location;
        const char *name;
    };

    shader_var_t uniforms[] = {
        {&loc.tex0,               "tex0"},
        {&loc.tex1,               "tex1"},
        {&loc.tex2,               "tex2"},
        {&loc.tex3,               "tex3"},
        {&loc.tex4,               "tex4"},
        {&loc.tex5,               "tex5"},
        {&loc.tex6,               "tex6"},
        {&loc.tex7,               "tex7"},
        {&loc.time,               "time"},
        {&loc.normalmap_strength, "normalmap_strength"},
        {&loc.glossiness,         "glossiness"},
        {&loc.specularlevel,      "specularlevel"},
        {&loc.uCameraPos,         "uCameraPos"},
        {&loc.uSpecularMaterial,  "uSpecularMaterial"},
        {&loc.uDynLightPos,       "uDynLightPos"},
        {&loc.uDynLightColor,     "uDynLightColor"},
        {&loc.uDynLightCount,     "uDynLightCount"},
        {&loc.uFogColor,          "uFogColor"},
        {&loc.uFogStart,          "uFogStart"},
        {&loc.uFogEnd,            "uFogEnd"},
        {&loc.uShadowMap,         "uShadowMap"},
        {&loc.uShadowMatrix,      "uShadowMatrix"},
        {&loc.uShadowActive,      "uShadowActive"},
    };
    const int num_uniforms = (int)(sizeof(uniforms) / sizeof(uniforms[0]));

    shader_var_t attribs[] = {
        {&loc.tangent, "tangent"},
    };

    // find locations for shader variables (-1 is fine for unused uniforms)
    for (int k = 0; k < num_uniforms; k++)
        *uniforms[k].location = glGetUniformLocation(prog_id, uniforms[k].name);
    PrintError(); // after glGetUniformLocation loop

    for (int k = 0; k < 1; k++)
        *attribs[k].location = glGetAttribLocation(prog_id, attribs[k].name);
    PrintError(); // after glGetAttribLocation
    // Note: loc.tangent == -1 is fine; shaders using derivative TBN don't need it.

    // Bind MatrixBlock uniform block to binding point 0
    GLuint block_idx = glGetUniformBlockIndex(prog_id, "MatrixBlock");
    PrintError(); // after glGetUniformBlockIndex
    if (block_idx != GL_INVALID_INDEX)
    {
        glUniformBlockBinding(prog_id, block_idx, 0);
        PrintError(); // after glUniformBlockBinding
    }

    // Validate after binding all samplers to their intended units so the driver
    // does not see a false sampler-type collision at unit 0.  glValidateProgram
    // inspects the current uniform state, so samplers must be wired first.
    // Intel's driver is strict: a type conflict generates GL_INVALID_VALUE in
    // the error queue (in addition to the info log), which then propagates to
    // the next drain point.  Only validate on successfully linked programs.
    if (status == GL_TRUE)
    {
        glUseProgram(prog_id);
        SetSamplerUniforms();
        PrintError(); // drain any errors from SetSamplerUniforms
        glValidateProgram(prog_id);
        PrintError(); // drain any GL error glValidateProgram placed in the queue
        PrintInfoLog();
        PrintError();
        glUseProgram(0);
    }
}

void ShaderProg::Use()
{
    glUseProgram(prog_id);
}

void ShaderProg::SetUniforms()
{
    // only call after linking!
    // set uniform vars (per-primitive vars, ie. only changed outside glBegin()..glEnd())
    glUniform1i(loc.tex0, 0);
    glUniform1i(loc.tex1, 1);
    if (oglrenderer->mp)
        glUniform1f(loc.time, oglrenderer->mp->maptic / 60.0);
    glUniform1f(loc.normalmap_strength, cv_grnormalmapstrength.value / 10.0f);
}

void ShaderProg::SetPerMaterialUniforms(float glossiness, float specularlevel)
{
    if (loc.glossiness    >= 0) glUniform1f(loc.glossiness,    glossiness);
    if (loc.specularlevel >= 0) glUniform1f(loc.specularlevel, specularlevel);
    if (loc.uSpecularMaterial >= 0)
    {
        float v[2] = {glossiness, specularlevel};
        glUniform2fv(loc.uSpecularMaterial, 1, v);
    }
}

void ShaderProg::SetDynamicLights(const float *eyePositions4, const float *colors3, int count)
{
    if (count < 0) count = 0;
    if (count > MAX_SHADER_LIGHTS) count = MAX_SHADER_LIGHTS;

    if (loc.uDynLightCount >= 0)
        glUniform1i(loc.uDynLightCount, count);
    if (count > 0)
    {
        if (loc.uDynLightPos   >= 0 && eyePositions4)
            glUniform4fv(loc.uDynLightPos,   count, eyePositions4);
        if (loc.uDynLightColor >= 0 && colors3)
            glUniform3fv(loc.uDynLightColor, count, colors3);
    }
}

void ShaderProg::SetFog(const float *color3, float start, float end)
{
    if (loc.uFogColor >= 0 && color3)
        glUniform3fv(loc.uFogColor, 1, color3);
    if (loc.uFogStart >= 0)
        glUniform1f(loc.uFogStart, start);
    if (loc.uFogEnd >= 0)
        glUniform1f(loc.uFogEnd, end);
}

void ShaderProg::SetShadow(GLuint shadow_tex_unit, const float *shadowMatrix16, int shadowActive)
{
    if (loc.uShadowActive >= 0)
        glUniform1i(loc.uShadowActive, shadowActive);
    if (shadowActive && shadowMatrix16)
    {
        if (loc.uShadowMatrix >= 0)
            glUniformMatrix4fv(loc.uShadowMatrix, 1, GL_FALSE, shadowMatrix16);
        if (loc.uShadowMap >= 0)
            glUniform1i(loc.uShadowMap, (GLint)shadow_tex_unit);
    }
}

void ShaderProg::SetSamplerUniforms()
{
    // Wire sampler uniforms to texture units. Safe to call before a map is loaded
    // (unlike SetUniforms which reads oglrenderer->mp->maptic).
    glUniform1i(loc.tex0, 0);
    glUniform1i(loc.tex1, 1);
    // PBR map units — glGetUniformLocation returns -1 if not in shader, glUniform1i ignores -1
    glUniform1i(loc.tex2, 2);
    glUniform1i(loc.tex3, 3);
    glUniform1i(loc.tex4, 4);
    glUniform1i(loc.tex5, 5);
    glUniform1i(loc.tex6, 6);
    glUniform1i(loc.tex7, 7);
    // Shadow sampler must always be wired to unit 8 so sampler2DShadow
    // never aliases the same unit as a sampler2D (tex0-tex7 on units 0-7).
    // Some Windows drivers crash or produce UB when two different sampler
    // types refer to the same texture image unit, even if the shadow branch
    // is not executed.
    if (loc.uShadowMap >= 0)
        glUniform1i(loc.uShadowMap, 8);
}

void ShaderProg::SetFpSamplerUniforms()
{
    // Wire GZDoom sampler names to our texture units (for .fp host programs).
    static const struct { const char *name; int unit; } map[] = {
        {"tex0",             0},
        {"normaltexture",    1},
        {"metallictexture",  2},
        {"roughnesstexture", 3},
        {"aotexture",        4},
        {"speculartexture",  5},
        {"brighttexture",    6},
        {"displacement",     7},
    };
    for (int i = 0; i < 8; i++)
    {
        GLint uloc = glGetUniformLocation(prog_id, map[i].name);
        if (uloc >= 0) glUniform1i(uloc, map[i].unit);
    }
}

void ShaderProg::SetAttributes(shader_attribs_t *a)
{
    if (loc.tangent >= 0)
        glVertexAttrib3fv(loc.tangent, a->tangent);
}

void ShaderProg::PrintInfoLog()
{
    int len = 0;
    glGetProgramiv(prog_id, GL_INFO_LOG_LENGTH, &len);

    if (len > 0)
    {
        char *log = static_cast<char *>(Z_Malloc(len, PU_DAVE, NULL));
        int chars = 0;
        glGetProgramInfoLog(prog_id, len, &chars, log);
        CONS_Printf("Shader program '%s' InfoLog: %s\n", name, log);
        Z_Free(log);
    }
}

// ---------------------------------------------------------------------------
// Built-in PBR normal-map shader (GLSL 1.30)
// Derivative-based TBN — no tangent vertex attribute required.
// Gracefully uses PBR map slots 2-7 when present (bound as neutral textures
// when absent, so the math is always well-defined).
// ---------------------------------------------------------------------------

static const char *normalmap_vert_src =
    "#version 330 compatibility\n"
    // Use GLSL compat built-ins directly instead of generic layout attributes:
    // gl_Vertex, gl_Normal, gl_Color, gl_MultiTexCoord0 are guaranteed to be
    // populated by glVertex3f / glNormal3f / glColor4f / glTexCoord2f in
    // immediate mode, avoiding driver attribute-aliasing bugs.
    // MatrixUBO (binding point 0) — provides matrices without glGetFloatv stalls
    "layout(std140) uniform MatrixBlock {\n"
    "    mat4 uModelView;\n"
    "    mat4 uProjection;\n"
    "    mat4 uMVP;\n"
    "};\n"
    "out vec3 v_eyedir;\n"
    "out vec2 v_texcoord;\n"
    "out vec4 v_color;\n"
    "out vec3 v_fragpos;\n"
    "out vec3 v_worldpos;\n"
    "void main()\n"
    "{\n"
    "    vec3 eyepos = (uModelView * gl_Vertex).xyz;\n"
    "    v_eyedir    = normalize(-eyepos);\n"
    "    v_fragpos   = eyepos;\n"
    "    v_worldpos  = gl_Vertex.xyz;\n"
    "    v_texcoord  = gl_MultiTexCoord0.xy;\n"
    "    v_color     = gl_Color;\n"
    "    gl_Position = uMVP * gl_Vertex;\n"
    "}\n";

static const char *normalmap_frag_src =
    "#version 330 compatibility\n"
    "layout(location=0) out vec4 fragColor;\n"
    // G-buffer MRT outputs (silently discarded when the corresponding attachment
    // is not bound, per the OpenGL spec — safe on the forward-only path).
    // Phase 3.1:  attachment 1 = view-space normal (packed [0,1])
    // Phase 3.1b: attachment 2 = pre-fog diffuse albedo (used by deferred shader)
    "layout(location=1) out vec4 gNormal;\n"
    "layout(location=2) out vec4 gAlbedo;\n"
    "uniform sampler2D tex0;\n"  // diffuse
    "uniform sampler2D tex1;\n"  // normal map
    "uniform sampler2D tex2;\n"  // metallic   (neutral: black=0)
    "uniform sampler2D tex3;\n"  // roughness  (neutral: gray=0.5)
    "uniform sampler2D tex4;\n"  // AO         (neutral: white=1)
    "uniform sampler2D tex5;\n"  // specular   (neutral: black=0)
    "uniform sampler2D tex6;\n"  // brightmap  (neutral: black=0)
    "uniform float normalmap_strength;\n"
    "uniform float glossiness;\n"
    "uniform float specularlevel;\n"
    // Dynamic point lights
    "#define MAX_SHADER_LIGHTS 16\n"
    "uniform vec4 uDynLightPos[MAX_SHADER_LIGHTS];\n"   // xyz=eye-space pos, w=radius
    "uniform vec3 uDynLightColor[MAX_SHADER_LIGHTS];\n"
    "uniform int  uDynLightCount;\n"
    "uniform vec3  uFogColor;\n"
    "uniform float uFogStart;\n"
    "uniform float uFogEnd;\n"
    "uniform sampler2DShadow uShadowMap;\n"
    "uniform mat4 uShadowMatrix;\n"
    "uniform int  uShadowActive;\n"
    "in vec3 v_eyedir;\n"
    "in vec2 v_texcoord;\n"
    "in vec4 v_color;\n"
    "in vec3 v_fragpos;\n"
    "in vec3 v_worldpos;\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = v_texcoord;\n"
    "    vec4 diffuse = texture(tex0, uv);\n"
    "    if (diffuse.a < 0.01) discard;\n"
    // Derivative-based TBN (cotangent frame)
    "    vec3 dp1  = dFdx(v_fragpos);\n"
    "    vec3 dp2  = dFdy(v_fragpos);\n"
    "    vec2 duv1 = dFdx(uv);\n"
    "    vec2 duv2 = dFdy(uv);\n"
    "    vec3 geo_N = normalize(cross(dp1, dp2));\n"
    "    if (dot(geo_N, -v_fragpos) < 0.0) geo_N = -geo_N;\n"
    "    vec3 dp2perp = cross(dp2, geo_N);\n"
    "    vec3 dp1perp = cross(geo_N, dp1);\n"
    "    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;\n"
    "    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;\n"
    "    float invmax = inversesqrt(max(dot(T,T), dot(B,B)));\n"
    "    mat3 TBN = mat3(T * invmax, B * invmax, geo_N);\n"
    // Normal map decode + strength
    "    vec3 tN = texture(tex1, uv).rgb * 2.0 - 1.0;\n"
    "    tN.xy *= normalmap_strength;\n"
    "    vec3 N = normalize(TBN * tN);\n"
    // Write packed view-space normal to G-buffer attachment 1 (Phase 3.1).
    // Packed as [0,1] from [-1,1] so it fits RGBA16F without sign issues.
    "    gNormal = vec4(N * 0.5 + 0.5, 1.0);\n"
    // Write pre-fog diffuse to G-buffer attachment 2 (Phase 3.1b).
    // diffuse.rgb only — v_color (sector brightness) is excluded so that
    // lights in dark sectors are still visible in the deferred pass.
    // Must be written BEFORE the fog mix below; discarded when attachment 2 absent.
    "    gAlbedo = vec4(diffuse.rgb, 1.0);\n"
    // PBR map samples (neutrals bound when slot is empty)
    "    float metallic  = texture(tex2, uv).r;\n"
    "    float roughness = texture(tex3, uv).r;\n"
    "    float ao        = texture(tex4, uv).r;\n"
    "    vec3  specTex   = texture(tex5, uv).rgb;\n"
    "    vec4  bright    = texture(tex6, uv);\n"
    // Static headlamp lighting (view direction as ambient)
    "    vec3 V = v_eyedir;\n"
    "    vec3 L = V;\n"
    "    float NdotL = max(dot(N, L), 0.0);\n"
    "    float gloss = max(glossiness, 1.0) * (1.0 - roughness * 0.8);\n"
    "    float smoothness = 1.0 - roughness;\n"
    "    vec3 specColor = mix(vec3(specularlevel * 0.25), specTex, metallic);\n"
    "    float spec = pow(NdotL, gloss) * specularlevel * 0.25;\n"
    "    vec3 color = diffuse.rgb * v_color.rgb;\n"
    "    color *= ao;\n"
    "    color  = color * (0.5 + 0.5 * NdotL) + specColor * spec;\n"
    // Dynamic point lights
    "    for (int i = 0; i < uDynLightCount; i++) {\n"
    "        vec3 Ldir = uDynLightPos[i].xyz - v_fragpos;\n"
    "        float dist = length(Ldir);\n"
    "        float atten = clamp(1.0 - dist / uDynLightPos[i].w, 0.0, 1.0);\n"
    "        atten *= atten;\n"
    "        float NdotLi = max(dot(N, normalize(Ldir)), 0.0);\n"
    "        color += diffuse.rgb * uDynLightColor[i] * NdotLi * atten;\n"
    "        vec3 H = normalize(normalize(Ldir) + V);\n"
    "        float dynspec = pow(max(dot(N, H), 0.0), max(glossiness * smoothness, 1.0))\n"
    "                        * specularlevel * smoothness;\n"
    "        color += specColor * uDynLightColor[i] * dynspec * atten;\n"
    "    }\n"
    "    color += bright.rgb * bright.a;\n"
    "    vec4 finalColor = vec4(color, diffuse.a * v_color.a);\n"
    // Shadow map sampling (PCF 2×2)
    "    if (uShadowActive != 0) {\n"
    "        vec4 sc = uShadowMatrix * vec4(v_worldpos, 1.0);\n"
    "        // Perspective divide + small depth bias to prevent self-shadowing\n"
    "        sc.z -= 0.005 * sc.w;\n"
    "        float shadow = 0.0;\n"
    "        vec2 texelSize = vec2(1.0 / 1024.0);\n"
    "        for (int sx = -1; sx <= 1; sx++) {\n"
    "            for (int sy = -1; sy <= 1; sy++) {\n"
    "                vec4 offset = vec4(float(sx) * texelSize.x * sc.w,\n"
    "                                  float(sy) * texelSize.y * sc.w, 0.0, 0.0);\n"
    "                shadow += textureProj(uShadowMap, sc + offset);\n"
    "            }\n"
    "        }\n"
    "        shadow /= 9.0;\n"
    "        // Darken shadowed areas — preserve a 30% ambient so shadows aren't pitch black\n"
    "        float shadowFactor = mix(0.3, 1.0, shadow);\n"
    "        finalColor.rgb *= shadowFactor;\n"
    "    }\n"
    "    if (uFogEnd > uFogStart) {\n"
    "        float dist = length(v_fragpos);\n"
    "        float fogFactor = clamp((uFogEnd - dist) / (uFogEnd - uFogStart), 0.0, 1.0);\n"
    "        finalColor.rgb = mix(uFogColor, finalColor.rgb, fogFactor);\n"
    "    }\n"
    "    fragColor = finalColor;\n"
    "}\n";

// ---------------------------------------------------------------------------
// Neutral 1x1 PBR textures — bound to slots 2-7 when a material lacks them
// ---------------------------------------------------------------------------

static GLuint s_neutral_normal = 0; // (128,128,255) flat tangent-space normal
static GLuint s_neutral_black  = 0; // metallic=0, specular=0, bright=0
static GLuint s_neutral_gray   = 0; // roughness=0.5, displacement=0.5
static GLuint s_neutral_white  = 0; // ao=1 (no occlusion)

static GLuint MakeNeutralTex(GLubyte r, GLubyte g, GLubyte b)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    GLubyte px[4] = {r, g, b, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

void DeletePBRNeutralTextures()
{
    if (s_neutral_normal) { glDeleteTextures(1, &s_neutral_normal); s_neutral_normal = 0; }
    if (s_neutral_black)  { glDeleteTextures(1, &s_neutral_black);  s_neutral_black  = 0; }
    if (s_neutral_gray)   { glDeleteTextures(1, &s_neutral_gray);   s_neutral_gray   = 0; }
    if (s_neutral_white)  { glDeleteTextures(1, &s_neutral_white);  s_neutral_white  = 0; }
}

void BindPBRNeutral(int slot)
{
    // Per-slot neutral:
    //   1=normal(flat 128,128,255), 2=metallic(black), 3=roughness(gray),
    //   4=ao(white), 5=specular(black), 6=brightmap(black), 7=displacement(gray)
    static const GLuint *map[] = {
        &s_neutral_normal, // 1: normal map (flat)
        &s_neutral_black,  // 2: metallic
        &s_neutral_gray,   // 3: roughness
        &s_neutral_white,  // 4: ao
        &s_neutral_black,  // 5: specular
        &s_neutral_black,  // 6: brightmap
        &s_neutral_gray,   // 7: displacement
    };
    if (slot < 1 || slot > 7) return;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, *map[slot - 1]);
}

void InitNormalMapShader()
{
    if (normalmap_shaderprog)
        return; // already compiled

    // Create neutral PBR textures (if not already present)
    if (!s_neutral_normal) s_neutral_normal = MakeNeutralTex(128, 128, 255); // flat normal
    if (!s_neutral_black)  s_neutral_black  = MakeNeutralTex(0,   0,   0);
    if (!s_neutral_gray)   s_neutral_gray   = MakeNeutralTex(128, 128, 128);
    if (!s_neutral_white)  s_neutral_white  = MakeNeutralTex(255, 255, 255);

    Shader *vs = new Shader("normalmap_vert", normalmap_vert_src, true);
    Shader *fs = new Shader("normalmap_frag", normalmap_frag_src, false);

    if (!vs->IsReady() || !fs->IsReady())
    {
        CONS_Printf("InitNormalMapShader: compilation failed, normal maps disabled.\n");
        delete vs;
        delete fs;
        return;
    }

    ShaderProg *prog = new ShaderProg("normalmap");
    prog->AttachShader(vs);
    prog->AttachShader(fs);
    prog->Link();

    // Wire all sampler uniforms (tex0-tex7 = units 0-7) while the program is active.
    prog->Use();
    prog->SetSamplerUniforms();
    ShaderProg::DisableShaders();

    normalmap_shaderprog = prog;
    CONS_Printf("InitNormalMapShader: compiled OK.\n");
}

// ---------------------------------------------------------------------------
// .fp host shader — wraps GZDoom ProcessMaterial() snippets
// ---------------------------------------------------------------------------

// Vertex shader: outputs the varyings the .fp files expect.
static const char *fp_host_vert_src =
    "#version 330 compatibility\n"
    // Use GLSL compat built-ins — same rationale as normalmap_vert_src.
    "layout(std140) uniform MatrixBlock {\n"
    "    mat4 uModelView;\n"
    "    mat4 uProjection;\n"
    "    mat4 uMVP;\n"
    "};\n"
    "out vec3 pixelpos;\n"
    "out vec2 vTexCoord;\n"
    "out vec4 vColor;\n"
    "void main()\n"
    "{\n"
    "    pixelpos    = (uModelView * gl_Vertex).xyz;\n"
    "    vTexCoord   = gl_MultiTexCoord0.xy;\n"
    "    vColor      = gl_Color;\n"
    "    gl_Position = uMVP * gl_Vertex;\n"
    "}\n";

// Fragment preamble: everything the .fp snippet expects from the host.
static const char *fp_host_frag_preamble =
    "#version 330 compatibility\n"
    // GZDoom-named samplers wired to our texture units
    "uniform sampler2D tex0;\n"             // unit 0: diffuse
    "uniform sampler2D normaltexture;\n"    // unit 1
    "uniform sampler2D metallictexture;\n"  // unit 2
    "uniform sampler2D roughnesstexture;\n" // unit 3
    "uniform sampler2D aotexture;\n"        // unit 4
    "uniform sampler2D speculartexture;\n"  // unit 5
    "uniform sampler2D brighttexture;\n"    // unit 6
    "uniform sampler2D displacement;\n"     // unit 7
    // Scalars
    "uniform vec2  uSpecularMaterial;\n"    // .x=glossiness .y=specularlevel
    "uniform vec3  uCameraPos;\n"           // camera pos (eye-space: always 0,0,0)
    // Dynamic point lights
    "#define MAX_SHADER_LIGHTS 16\n"
    "uniform vec4 uDynLightPos[MAX_SHADER_LIGHTS];\n"
    "uniform vec3 uDynLightColor[MAX_SHADER_LIGHTS];\n"
    "uniform int  uDynLightCount;\n"
    "uniform vec3  uFogColor;\n"
    "uniform float uFogStart;\n"
    "uniform float uFogEnd;\n"
    // Varyings (no vWorldNormal from vertex — computed analytically below)
    // MRT outputs (silently discarded when attachment absent — same as normalmap shader)
    "layout(location=0) out vec4 fragColor;\n"
    "layout(location=1) out vec4 gNormal;\n"   // view-space normal packed [0,1] (Phase 3.3)
    "layout(location=2) out vec4 gAlbedo;\n"   // pre-fog albedo (Phase 3.1b)
    "in vec3 pixelpos;\n"
    "in vec2 vTexCoord;\n"
    "in vec4 vColor;\n"
    // vWorldNormal computed from screen-space derivatives (works even if gl_Normal not set).
    // The .fp files access vWorldNormal.xyz — the macro expands every use to this call.
    "vec4 vWorldNormal_fn() {\n"
    "    vec3 dp1 = dFdx(pixelpos);\n"
    "    vec3 dp2 = dFdy(pixelpos);\n"
    // Raw cross product — no flip.  GetTBN() constructs T,B,N from the same
    // dp1/dp2, so the TBN handedness is self-consistent with this normal.
    // Flipping n here would negate all three TBN axes, inverting both the
    // normal map and the parallax walk direction.
    "    return vec4(-normalize(cross(dp1, dp2)), 1.0);\n"
    "}\n"
    "#define vWorldNormal (vWorldNormal_fn())\n"
    // GZDoom Material struct
    "struct Material {\n"
    "    vec4 Base;\n"
    "    vec3 Normal;\n"
    "    vec3 Specular;\n"
    "    float Glossiness;\n"
    "    float SpecularLevel;\n"
    "    float Metallic;\n"
    "    float Roughness;\n"
    "    float AO;\n"
    "    vec4 Bright;\n"
    "};\n"
    // Host function called by .fp via getTexel()
    "vec4 getTexel(vec2 uv) { return texture(tex0, uv); }\n"
    // Enable all PBR features; neutral textures ensure safe defaults when maps absent
    "#define NORMALMAP\n"
    "#define PBR\n"
    "#define SPECULAR\n"
    "#define BRIGHTMAP\n"
    // ---- .fp snippet will be appended here ----
    ;

// Fragment epilogue: main() applies lighting to the Material returned by ProcessMaterial().
// Fragment epilogue: apply Doom-style lighting to the Material from ProcessMaterial().
// Uses half-Lambert off the view direction so normal maps add bump detail without
// black shadow bands.  Sector brightness comes from the vertex colour (vColor.rgb).
static const char *fp_host_frag_epilogue =
    "void main()\n"
    "{\n"
    "    Material mat = ProcessMaterial();\n"
    "\n"
    "    vec3 N = normalize(mat.Normal);\n"
    "    vec3 V = normalize(-pixelpos);\n"
    "\n"
    "    // Half-Lambert off |NdotV|: the raw derivative normal may point either\n"
    "    // toward or away from the camera depending on the platform's dFdy sign\n"
    "    // convention, so use abs() to keep diffuse in [0.5..1.0] regardless.\n"
    "    float NdotV = abs(dot(N, V));\n"
    "    float diffuse = 0.5 + 0.5 * NdotV;\n"
    "\n"
    "    // AO: 1.0 = unoccluded, 0.0 = fully occluded (standard PBR).  Neutral\n"
    "    // white texture keeps AO at 1.0 (no effect) when no map is provided.\n"
    "    vec3 color = mat.Base.rgb * vColor.rgb * diffuse * mat.AO;\n"
    "\n"
    "    // Specular highlight (Blinn-Phong, camera as light source so H = V).\n"
    "    // Roughness widens/dims the highlight; metallic blends specular map with base.\n"
    "    float smoothness = 1.0 - mat.Roughness;\n"
    "    float spec = pow(NdotV, max(mat.Glossiness * smoothness, 1.0))\n"
    "                 * mat.SpecularLevel * smoothness;\n"
    "    vec3 specColor = mix(mat.Specular, mat.Base.rgb, mat.Metallic);\n"
    "    color += specColor * spec * vColor.rgb;\n"
    "\n"
    "    // Dynamic point lights\n"
    "    for (int i = 0; i < uDynLightCount; i++) {\n"
    "        vec3 Ldir = uDynLightPos[i].xyz - pixelpos;\n"
    "        float dist = length(Ldir);\n"
    "        float atten = clamp(1.0 - dist / uDynLightPos[i].w, 0.0, 1.0);\n"
    "        atten *= atten;\n"
    "        float NdotLi = max(dot(N, normalize(Ldir)), 0.0);\n"
    "        color += mat.Base.rgb * uDynLightColor[i] * NdotLi * atten;\n"
    "        vec3 H = normalize(normalize(Ldir) + V);\n"
    "        float dynspec = pow(max(dot(N, H), 0.0), max(mat.Glossiness * smoothness, 1.0))\n"
    "                        * mat.SpecularLevel * smoothness;\n"
    "        color += specColor * uDynLightColor[i] * dynspec * atten;\n"
    "    }\n"
    "\n"
    "    // Brightmap: additive emissive, unaffected by sector light or normals.\n"
    "    color += mat.Bright.rgb * mat.Bright.a;\n"
    "\n"
    "    vec4 finalColor = vec4(color, mat.Base.a * vColor.a);\n"
    // Write view-space normal to G-buffer attachment 1 (Phase 3.3).
    // N is already computed above (normalize(mat.Normal), which is a view-space derivative
    // normal for the host shader path). Pack into [0,1] to match the normalmap shader.
    "    gNormal = vec4(N * 0.5 + 0.5, 1.0);\n"
    // Write pre-fog albedo to G-buffer attachment 2 (Phase 3.1b).
    // mat.Base.rgb is the raw diffuse texture color before fog or lighting tint.
    "    gAlbedo = vec4(mat.Base.rgb, 1.0);\n"
    "    if (uFogEnd > uFogStart) {\n"
    "        float dist = length(pixelpos);\n"
    "        float fogFactor = clamp((uFogEnd - dist) / (uFogEnd - uFogStart), 0.0, 1.0);\n"
    "        finalColor.rgb = mix(uFogColor, finalColor.rgb, fogFactor);\n"
    "    }\n"
    "    fragColor = finalColor;\n"
    "}\n";

// Cache: fp_lump_path -> compiled ShaderProg
static std::map<std::string, ShaderProg *> fp_shader_cache;

ShaderProg *CompileFpShader(const char *fp_lump_path)
{
    // Check cache first
    std::string key(fp_lump_path);
    std::map<std::string, ShaderProg *>::iterator it = fp_shader_cache.find(key);
    if (it != fp_shader_cache.end())
        return it->second;

    // Find and read the .fp lump
    int lump = -1;
    int nfiles = (int)fc.Size();
    for (int fi = nfiles - 1; fi >= 0 && lump < 0; fi--)
    {
        int nlumps = (int)fc.GetNumLumps(fi);
        for (int j = 0; j < nlumps && lump < 0; j++)
        {
            int candidate = (fi << 16) | j;
            const char *n = fc.FindNameForNum(candidate);
            if (n && strcasecmp(n, fp_lump_path) == 0)
                lump = candidate;
        }
    }
    if (lump < 0)
    {
        CONS_Printf("CompileFpShader: lump '%s' not found.\n", fp_lump_path);
        fp_shader_cache[key] = NULL;
        return NULL;
    }

    int fp_len = fc.LumpLength(lump);
    char *fp_src = static_cast<char *>(Z_Malloc(fp_len + 1, PU_DAVE, NULL));
    fc.ReadLump(lump, fp_src);
    fp_src[fp_len] = '\0';

    // Build fragment source: preamble + .fp content + epilogue
    size_t total = strlen(fp_host_frag_preamble) + fp_len + strlen(fp_host_frag_epilogue) + 1;
    char *frag_src = static_cast<char *>(Z_Malloc(total, PU_DAVE, NULL));
    strcpy(frag_src, fp_host_frag_preamble);
    strcat(frag_src, fp_src);
    strcat(frag_src, fp_host_frag_epilogue);
    Z_Free(fp_src);

    // Compile
    Shader *vs = new Shader("fp_host_vert", fp_host_vert_src, true);
    Shader *fs = new Shader(fp_lump_path,   frag_src,          false);
    Z_Free(frag_src);

    if (!vs->IsReady() || !fs->IsReady())
    {
        CONS_Printf("CompileFpShader: '%s' failed to compile.\n", fp_lump_path);
        delete vs;
        delete fs;
        fp_shader_cache[key] = NULL;
        return NULL;
    }

    std::string prog_name = std::string("fp:") + fp_lump_path;
    ShaderProg *prog = new ShaderProg(prog_name.c_str());
    prog->AttachShader(vs);
    prog->AttachShader(fs);
    prog->Link();

    // Wire GZDoom sampler names to texture units
    prog->Use();
    prog->SetFpSamplerUniforms();
    ShaderProg::DisableShaders();

    fp_shader_cache[key] = prog;
    CONS_Printf("CompileFpShader: '%s' compiled OK.\n", fp_lump_path);
    return prog;
}

void ClearFpShaders()
{
    for (std::map<std::string, ShaderProg *>::iterator it = fp_shader_cache.begin();
         it != fp_shader_cache.end(); ++it)
    {
        delete it->second;
    }
    fp_shader_cache.clear();
}

void CompileGLDEFSShaders()
{
    if (!normalmap_shaderprog)
        return; // no GL context yet

    int compiled = 0, fallback = 0;
    const std::vector<Material *> &mats = materials.GetAllMaterials();
    for (size_t mi = 0; mi < mats.size(); mi++)
    {
        Material *m = mats[mi];
        if (!m || m->shader_lump.empty())
            continue;

        ShaderProg *prog = CompileFpShader(m->shader_lump.c_str());
        if (prog)
        {
            m->shader = prog;
            compiled++;
        }
        else
        {
            m->shader = normalmap_shaderprog;
            fallback++;
        }
    }
    if (compiled || fallback)
        CONS_Printf("CompileGLDEFSShaders: %d .fp shaders, %d fallback.\n", compiled, fallback);
}

#endif // GL_VERSION_2_0
