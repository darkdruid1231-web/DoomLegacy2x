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
#include "hardware/hw_gbuffer.h"
#include "hardware/oglshaders.h"
#include "command.h"   // full consvar_t definition
#include "cvars.h"
#include "console.h"
#include "core/doomdef.h"  // devparm
#include <cmath>
#include <cstring>
#include <cstdlib>

PostFX postfx;

/// 1×1 fully-transparent RGBA16F texture used as the null binding for the uVolFog
/// sampler when volumetric fog is disabled.  Avoids sampling the incomplete default
/// texture (GL name 0), which returns (0,0,0,1) and blacks out the entire frame.
static GLuint s_null_fog_tex = 0;

// ---------------------------------------------------------------------------
// Simple vertex shader reused by all fullscreen-quad passes.
// gl_MultiTexCoord0.xy is 0..1; vertex position is derived from it.
// ---------------------------------------------------------------------------
static const char *postfx_vert_src =
    "#version 330 compatibility\n"
    "layout(location=0) in vec2 aTexCoord;\n"
    "out vec2 vTexCoord;\n"
    "void main() {\n"
    "    vTexCoord = aTexCoord;\n"
    "    gl_Position = vec4(aTexCoord * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Bloom: bright-pass
// ---------------------------------------------------------------------------
static const char *bloom_brightpass_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uScene;\n"
    "uniform float uThreshold;\n"
    "uniform float uStrength;\n"
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "void main() {\n"
    "    vec3 c = texture(uScene, vTexCoord).rgb;\n"
    "    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    "    float knee = 0.1;\n"
    "    float w = smoothstep(uThreshold - knee, uThreshold + knee, lum);\n"
    "    fragColor = vec4(c * w * uStrength, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Bloom: separable Gaussian blur (7-tap, bilinear-optimised)
// uDir should be (1/texWidth, 0) for horizontal, (0, 1/texHeight) for vertical.
// ---------------------------------------------------------------------------
static const char *bloom_blur_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uBloom;\n"
    "uniform vec2 uDir;\n"
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
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
    "    fragColor = vec4(c, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Bloom: composite scene + bloom
// ---------------------------------------------------------------------------
static const char *bloom_composite_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uScene;\n"
    "uniform sampler2D uBloom;\n"
    "uniform sampler2D uLightAccum;\n"  // deferred accumulated lights (black when unused)
    "uniform sampler2D uSSR;\n"         // screen-space reflections (Phase 4.1); black when unused
    "uniform float uSSRStrength;\n"
    "uniform sampler2D uVolFog;\n"      // volumetric fog (Phase 4.2); transparent when unused
    "uniform sampler2D uGodRays;\n"     // god rays accumulation (Phase 5.3); black when unused
    "uniform float uGodRaysStrength;\n" // 0 = disabled
    "uniform float uBloomIntensity;\n"
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "void main() {\n"
    // Add deferred light accumulation, SSR, and god rays to scene color.
    "    vec3 scene = texture(uScene, vTexCoord).rgb + texture(uLightAccum, vTexCoord).rgb;\n"
    "    vec4 ssr = texture(uSSR, vTexCoord);\n"
    "    scene += ssr.rgb * ssr.a * uSSRStrength;\n"
    "    scene += texture(uGodRays, vTexCoord).rgb * uGodRaysStrength;\n"
    "    vec3 bloom = texture(uBloom, vTexCoord).rgb;\n"
    "    vec3 result = scene + bloom * uBloomIntensity;\n"
    // Reinhard to roll off bloom highlights without hard clipping.
    "    result = result / (result + vec3(1.0));\n"
    // Volumetric fog applied after tonemapping.
    "    vec4 fog = texture(uVolFog, vTexCoord);\n"
    "    result = result * (1.0 - fog.a) + fog.rgb;\n"
    "    fragColor = vec4(result, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// SSAO: reconstruct view-space position from depth, sample hemisphere
// ---------------------------------------------------------------------------
static const char *ssao_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uDepth;\n"
    "uniform mat4 uProjInv;\n"
    "uniform mat4 uProj;\n"
    "uniform vec3 uKernel[16];\n"
    "uniform float uRadius;\n"
    "uniform float uBias;\n"
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
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
    "        // Project sample to screen UV via the full projection matrix.\n"
    "        vec4 offset = uProj * vec4(sampleVS, 1.0);\n"
    "        vec2 sampleUV = (offset.xy / offset.w) * 0.5 + 0.5;\n"
    "        sampleUV = clamp(sampleUV, 0.001, 0.999);\n"
    "        vec3 sampleActual = ReconstructPos(sampleUV);\n"
    "        float rangeCheck = smoothstep(0.0, 1.0,\n"
    "                           uRadius / max(abs(fragPos.z - sampleActual.z), 0.001));\n"
    "        occlusion += (sampleActual.z >= sampleVS.z + uBias ? 1.0 : 0.0) * rangeCheck;\n"
    "    }\n"
    "    float ao = 1.0 - (occlusion / 16.0);\n"
    "    fragColor = vec4(ao, ao, ao, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// SSAO blur: 3x3 box
// ---------------------------------------------------------------------------
static const char *ssao_blur_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uSSAOTex;\n"
    "uniform vec2 uTexelSize;\n"
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "void main() {\n"
    "    float result = 0.0;\n"
    "    for (int x = -1; x <= 1; x++) {\n"
    "        for (int y = -1; y <= 1; y++) {\n"
    "            vec2 off = vec2(float(x), float(y)) * uTexelSize;\n"
    "            result += texture(uSSAOTex, vTexCoord + off).r;\n"
    "        }\n"
    "    }\n"
    "    result /= 9.0;\n"
    "    fragColor = vec4(result, result, result, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// SSAO composite: multiply scene color by AO factor
// ---------------------------------------------------------------------------
static const char *ssao_composite_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uScene;\n"
    "uniform sampler2D uSSAOBlur;\n"
    "uniform float uSSAOStrength;\n"
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "void main() {\n"
    "    vec3 scene = texture(uScene, vTexCoord).rgb;\n"
    "    float ao = texture(uSSAOBlur, vTexCoord).r;\n"
    "    float factor = mix(1.0, ao, uSSAOStrength);\n"
    "    fragColor = vec4(scene * factor, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// SSR (Phase 4.1): screen-space reflections via ray-march + binary refinement.
// Requires G-buffer normals (gbuf_norm_tex) and scene depth.
// Outputs: rgb = reflected scene color, a = per-pixel confidence [0,1].
// The composite pass blends this additively: scene + ssr.rgb * ssr.a * strength.
// ---------------------------------------------------------------------------
static const char *ssr_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uDepth;\n"
    "uniform sampler2D uNormal;\n"      // G-buffer view-space normals packed [0,1]
    "uniform sampler2D uScene;\n"       // scene color to sample reflections from
    "uniform mat4 uProjInv;\n"
    "uniform mat4 uProj;\n"
    "uniform float uMaxDist;\n"         // max ray length in view-space units
    "uniform float uThickness;\n"       // depth tolerance for hit detection
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "\n"
    "vec3 ReconstructPos(vec2 uv) {\n"
    "    float d = texture(uDepth, uv).r;\n"
    "    vec4 clip = vec4(uv * 2.0 - 1.0, d * 2.0 - 1.0, 1.0);\n"
    "    vec4 view = uProjInv * clip;\n"
    "    return view.xyz / view.w;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    float rawDepth = texture(uDepth, vTexCoord).r;\n"
    "    if (rawDepth >= 0.9999) { fragColor = vec4(0.0); return; }\n"  // sky/background
    "\n"
    "    vec3 P = ReconstructPos(vTexCoord);\n"
    "    vec3 N = normalize(texture(uNormal, vTexCoord).rgb * 2.0 - 1.0);\n"
    "    vec3 V = normalize(-P);\n"
    "    vec3 R = reflect(-V, N);\n"    // view-space reflected direction
    "\n"
    "    if (R.z > 0.0) { fragColor = vec4(0.0); return; }\n"  // reflected toward camera
    "\n"
    "    float NdotV = clamp(dot(N, V), 0.0, 1.0);\n"
    "\n"
    "    const int STEPS = 32;\n"
    "    float stepLen = uMaxDist / float(STEPS);\n"
    "\n"
    "    for (int i = 1; i <= STEPS; i++) {\n"
    "        vec3 S = P + R * (float(i) * stepLen);\n"
    "        vec4 clip = uProj * vec4(S, 1.0);\n"
    "        vec2 uv = (clip.xy / clip.w) * 0.5 + 0.5;\n"
    "        if (any(lessThan(uv, vec2(0.01))) || any(greaterThan(uv, vec2(0.99)))) break;\n"
    "\n"
    "        vec3 A = ReconstructPos(uv);\n"
    "        float diff = A.z - S.z;\n"    // positive if actual geometry is closer to camera
    "\n"
    "        if (diff > 0.0 && diff < uThickness) {\n"
    "            // Binary refinement: 4 steps to sharpen the hit point\n"
    "            float tLo = float(i - 1) * stepLen;\n"
    "            float tHi = float(i)     * stepLen;\n"
    "            for (int j = 0; j < 4; j++) {\n"
    "                float tMid = (tLo + tHi) * 0.5;\n"
    "                vec4  cMid = uProj * vec4(P + R * tMid, 1.0);\n"
    "                vec2  uvMid = (cMid.xy / cMid.w) * 0.5 + 0.5;\n"
    "                vec3  sMid  = ReconstructPos(uvMid);\n"
    "                if (sMid.z > (P + R * tMid).z) tHi = tMid; else tLo = tMid;\n"
    "            }\n"
    "            vec4 cFinal = uProj * vec4(P + R * tLo, 1.0);\n"
    "            vec2 uvFinal = clamp((cFinal.xy / cFinal.w) * 0.5 + 0.5, 0.0, 1.0);\n"
    "\n"
    "            vec3 hitColor = texture(uScene, uvFinal).rgb;\n"
    "\n"
    "            // Edge fade: avoid hard cutoffs at screen border\n"
    "            vec2 edgeFade = smoothstep(0.0, 0.1, uvFinal)\n"
    "                          * smoothstep(1.0, 0.9, uvFinal);\n"
    "            float conf = edgeFade.x * edgeFade.y;\n"
    "            // Schlick Fresnel: more reflective at grazing angles\n"
    "            conf *= 0.05 + 0.95 * pow(1.0 - NdotV, 3.0);\n"
    "\n"
    "            fragColor = vec4(hitColor, conf);\n"
    "            return;\n"
    "        }\n"
    "    }\n"
    "    fragColor = vec4(0.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Volumetric fog (Phase 4.2)
// Analytical exponential fog integrated from camera to each fragment.
// Transmittance:  T(d) = exp(-density * d)
// In-scatter:     L    = fogColor * (1 - T)
// Composite:      finalColor = scene * T + fogColor * (1 - T)
//               = scene * (1 - fogOpacity) + fogColor * fogOpacity
// Outputs rgba where rgb = fogColor * (1-T) and a = (1-T) — the fog opacity.
// The composite pass does:  hdr = hdr * (1 - fog.a) + fog.rgb
// ---------------------------------------------------------------------------
static const char *vol_fog_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uDepth;\n"
    "uniform mat4 uProjInv;\n"
    "uniform vec3 uFogColor;\n"
    "uniform float uFogDensity;\n"  // extinction coefficient per view-space unit
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "\n"
    "void main() {\n"
    "    float d = texture(uDepth, vTexCoord).r;\n"
    "    if (d >= 0.9999) { fragColor = vec4(0.0); return; }\n"  // sky - no fog
    "\n"
    // Reconstruct linear view-space depth (positive distance from camera).
    "    vec4 clip = vec4(vTexCoord * 2.0 - 1.0, d * 2.0 - 1.0, 1.0);\n"
    "    vec4 view = uProjInv * clip;\n"
    "    float depth = abs(view.z / view.w);\n"
    "\n"
    // Analytical exponential integral along the view ray.
    "    float T       = exp(-uFogDensity * depth);\n"
    "    float opacity = 1.0 - T;\n"
    "\n"
    "    fragColor = vec4(uFogColor * opacity, opacity);\n"
    "}\n";

// ---------------------------------------------------------------------------
// SSAO with G-buffer normals (Phase 2.2): samples the normal texture from
// the MRT G-buffer instead of reconstructing via dFdx/dFdy derivatives.
// This eliminates the faceting artefacts visible at polygon edges.
// ---------------------------------------------------------------------------
static const char *ssao_gbuf_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uDepth;\n"
    "uniform sampler2D uNormal;\n"   // G-buffer: view-space normals packed [0,1]
    "uniform mat4 uProjInv;\n"
    "uniform mat4 uProj;\n"
    "uniform vec3 uKernel[16];\n"
    "uniform float uRadius;\n"
    "uniform float uBias;\n"
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
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
    "    // Read view-space normal from G-buffer (stored as N*0.5+0.5)\n"
    "    vec3 normal = normalize(texture(uNormal, vTexCoord).rgb * 2.0 - 1.0);\n"
    "    if (dot(normal, -fragPos) < 0.0) normal = -normal;\n"
    "\n"
    "    // Per-pixel rotation using screen-space hash to reduce banding\n"
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
    "        // Project sample to screen UV via the full projection matrix.\n"
    "        vec4 offset = uProj * vec4(sampleVS, 1.0);\n"
    "        vec2 sampleUV = (offset.xy / offset.w) * 0.5 + 0.5;\n"
    "        sampleUV = clamp(sampleUV, 0.001, 0.999);\n"
    "        vec3 sampleActual = ReconstructPos(sampleUV);\n"
    "        float rangeCheck = smoothstep(0.0, 1.0,\n"
    "                           uRadius / max(abs(fragPos.z - sampleActual.z), 0.001));\n"
    "        occlusion += (sampleActual.z >= sampleVS.z + uBias ? 1.0 : 0.0) * rangeCheck;\n"
    "    }\n"
    "    float ao = 1.0 - (occlusion / 16.0);\n"
    "    fragColor = vec4(ao, ao, ao, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Tone-mapping fragment shader (Phase 2.1 HDR)
// ---------------------------------------------------------------------------
static const char *tonemapping_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uHDR;\n"
    "uniform sampler2D uLightAccum;\n"  // deferred accumulated lights (black when unused)
    "uniform sampler2D uSSR;\n"         // screen-space reflections (Phase 4.1); black when unused
    "uniform float uSSRStrength;\n"
    "uniform sampler2D uVolFog;\n"      // volumetric fog (Phase 4.2); transparent when unused
    "uniform sampler2D uGodRays;\n"     // god rays accumulation (Phase 5.3); black when unused
    "uniform float uGodRaysStrength;\n" // 0 = disabled
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "void main() {\n"
    // Combine scene color with deferred light accumulation, SSR, and god rays.
    "    vec3 hdr = texture(uHDR, vTexCoord).rgb + texture(uLightAccum, vTexCoord).rgb;\n"
    "    vec4 ssr = texture(uSSR, vTexCoord);\n"
    "    hdr += ssr.rgb * ssr.a * uSSRStrength;\n"
    "    hdr += texture(uGodRays, vTexCoord).rgb * uGodRaysStrength;\n"
    // Volumetric fog: attenuate scene by transmittance, add fog inscatter.
    "    vec4 fog = texture(uVolFog, vTexCoord);\n"
    "    hdr = hdr * (1.0 - fog.a) + fog.rgb;\n"
    "    fragColor = vec4(clamp(hdr, 0.0, 1.0), 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// God rays (Phase 5.3): screen-space volumetric light shafts.
// Radial blur technique from GPU Gems 3 Ch.13 (Shawn O'Neil).
// Marches NUM_SAMPLES steps from the current pixel toward the projected sun
// position, accumulating decaying scene-color samples.  The result is blended
// additively over the final composite so it brightens areas along light paths.
// ---------------------------------------------------------------------------
static const char *godrays_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uScene;\n"   // HDR scene color (scene_color_tex)
    "uniform vec2 uLightPos;\n"     // sun position in [0,1] screen space
    "uniform float uDensity;\n"     // step scale along ray (0.9)
    "uniform float uDecay;\n"       // per-step luminance decay (0.96)
    "uniform float uWeight;\n"      // per-step sample weight (0.01)
    "uniform float uExposure;\n"    // final exposure multiplier (strength)
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "\n"
    "const int NUM_SAMPLES = 32;\n"
    "\n"
    "void main() {\n"
    "    vec2 delta = (vTexCoord - uLightPos) * (uDensity / float(NUM_SAMPLES));\n"
    "    vec2 uv    = vTexCoord;\n"
    "    float decay = 1.0;\n"
    "    vec3  color = vec3(0.0);\n"
    "    for (int i = 0; i < NUM_SAMPLES; i++) {\n"
    "        uv -= delta;\n"               // step toward light source
    "        vec3 s = texture(uScene, clamp(uv, 0.0, 1.0)).rgb;\n"
    // Soft-clip bright HDR samples so very intense sources don't wash out
    "        s = s / (s + vec3(1.0));\n"
    "        color += s * (decay * uWeight);\n"
    "        decay *= uDecay;\n"
    "    }\n"
    "    fragColor = vec4(color * uExposure, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// FXAA (Phase 5.2): luma-based edge detection + sub-pixel blend.
// Input: LDR scene color after tonemapping/bloom.  Output: anti-aliased scene.
// Uses 9-tap sampling (NSEW + 4 diagonals + center) with a Sobel-style edge
// direction test and a contrast-proportional sub-pixel blend weight.
// ---------------------------------------------------------------------------
static const char *fxaa_frag_src =
    "#version 330 compatibility\n"
    "uniform sampler2D uScene;\n"
    "uniform vec2 uTexelSize;\n"    // (1/width, 1/height)
    "in vec2 vTexCoord;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "\n"
    // Rec.709 luma weights.
    "#define LUMA(c) dot((c), vec3(0.2126, 0.7152, 0.0722))\n"
    "\n"
    "void main() {\n"
    "    vec2 uv = vTexCoord;\n"
    "    vec2 ts = uTexelSize;\n"
    "\n"
    // 5-tap cardinal samples for edge contrast.
    "    vec3 rgbM = texture(uScene, uv).rgb;\n"
    "    vec3 rgbN = texture(uScene, uv + vec2( 0.0,  ts.y)).rgb;\n"
    "    vec3 rgbS = texture(uScene, uv + vec2( 0.0, -ts.y)).rgb;\n"
    "    vec3 rgbE = texture(uScene, uv + vec2( ts.x,  0.0)).rgb;\n"
    "    vec3 rgbW = texture(uScene, uv + vec2(-ts.x,  0.0)).rgb;\n"
    "\n"
    "    float lumaM = LUMA(rgbM);\n"
    "    float lumaN = LUMA(rgbN);\n"
    "    float lumaS = LUMA(rgbS);\n"
    "    float lumaE = LUMA(rgbE);\n"
    "    float lumaW = LUMA(rgbW);\n"
    "\n"
    "    float lumaMin   = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));\n"
    "    float lumaMax   = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));\n"
    "    float lumaRange = lumaMax - lumaMin;\n"
    "\n"
    // Skip pixels below the contrast threshold — preserves sharp detail.
    "    if (lumaRange < max(0.0625, lumaMax * 0.125)) {\n"
    "        fragColor = vec4(rgbM, 1.0);\n"
    "        return;\n"
    "    }\n"
    "\n"
    // Diagonal taps for Sobel-style H/V edge detection.
    "    float lumaNW = LUMA(texture(uScene, uv + vec2(-ts.x,  ts.y)).rgb);\n"
    "    float lumaNE = LUMA(texture(uScene, uv + vec2( ts.x,  ts.y)).rgb);\n"
    "    float lumaSW = LUMA(texture(uScene, uv + vec2(-ts.x, -ts.y)).rgb);\n"
    "    float lumaSE = LUMA(texture(uScene, uv + vec2( ts.x, -ts.y)).rgb);\n"
    "\n"
    "    float edgeH = abs(-2.0*lumaW + lumaNW + lumaSW)\n"
    "                + abs(-2.0*lumaM + lumaN  + lumaS ) * 2.0\n"
    "                + abs(-2.0*lumaE + lumaNE + lumaSE);\n"
    "    float edgeV = abs(-2.0*lumaS + lumaSW + lumaSE)\n"
    "                + abs(-2.0*lumaM + lumaW  + lumaE ) * 2.0\n"
    "                + abs(-2.0*lumaN + lumaNW + lumaNE);\n"
    "    bool isHoriz = edgeH >= edgeV;\n"
    "\n"
    // Step one pixel perpendicular to the edge.
    "    vec2 stepDir = isHoriz ? vec2(0.0, ts.y) : vec2(ts.x, 0.0);\n"
    "\n"
    // Flip toward the side with the stronger gradient.
    "    float luma1 = isHoriz ? lumaN : lumaE;\n"
    "    float luma2 = isHoriz ? lumaS : lumaW;\n"
    "    if (abs(luma1 - lumaM) < abs(luma2 - lumaM)) stepDir = -stepDir;\n"
    "\n"
    // Sub-pixel blend weight: proportional to local contrast, clamped to 0.75.
    "    float lumaAvg = (lumaN + lumaS + lumaE + lumaW) * 0.25;\n"
    "    float blend   = clamp(abs(lumaAvg - lumaM) / lumaRange, 0.0, 1.0);\n"
    "    blend = blend * blend * 0.75;\n"
    "\n"
    "    fragColor = vec4(mix(rgbM, texture(uScene, uv + stepDir * blend).rgb, blend), 1.0);\n"
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
    , scene_fbo(0), scene_color_tex(0), scene_depth_tex(0), gbuf_norm_tex(0), gbuf_albedo_tex(0)
    , ssao_fbo(0), ssao_tex(0), ssao_blur_fbo(0), ssao_blur_tex(0)
    , ssr_fbo(0), ssr_tex(0)
    , volfog_fbo(0), volfog_tex(0)
    , ldr_fbo(0), ldr_tex(0)
    , godrays_fbo(0), godrays_tex(0)
    , screenquad_vao(0), screenquad_vbo(0)
    , prog_tonemapping(NULL)
    , prog_brightpass(NULL), prog_blur_h(NULL), prog_blur_v(NULL)
    , prog_composite(NULL), prog_ssao(NULL), prog_ssao_gbuf(NULL)
    , prog_ssao_blur(NULL), prog_ssao_composite(NULL)
    , prog_ssr(NULL), prog_vol_fog(NULL), prog_fxaa(NULL), prog_godrays(NULL)
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
    prog_ssao_gbuf      = MakePostFXProg("postfx_ssao_gbuf",      ssao_gbuf_frag_src);
    prog_ssao_blur      = MakePostFXProg("postfx_ssao_blur",      ssao_blur_frag_src);
    prog_ssao_composite = MakePostFXProg("postfx_ssao_composite", ssao_composite_frag_src);
    prog_ssr            = MakePostFXProg("postfx_ssr",            ssr_frag_src);
    prog_vol_fog        = MakePostFXProg("postfx_vol_fog",        vol_fog_frag_src);
    prog_fxaa           = MakePostFXProg("postfx_fxaa",           fxaa_frag_src);
    prog_godrays        = MakePostFXProg("postfx_godrays",        godrays_frag_src);
    prog_tonemapping    = MakePostFXProg("postfx_tonemapping",    tonemapping_frag_src);
}

void PostFX::DeleteShaders()
{
    delete prog_brightpass;     prog_brightpass     = NULL;
    delete prog_blur_h;         prog_blur_h         = NULL;
    delete prog_blur_v;         prog_blur_v         = NULL;
    delete prog_composite;      prog_composite      = NULL;
    delete prog_ssao;           prog_ssao           = NULL;
    delete prog_ssao_gbuf;      prog_ssao_gbuf      = NULL;
    delete prog_ssao_blur;      prog_ssao_blur      = NULL;
    delete prog_ssao_composite; prog_ssao_composite = NULL;
    delete prog_ssr;            prog_ssr            = NULL;
    delete prog_vol_fog;        prog_vol_fog        = NULL;
    delete prog_fxaa;           prog_fxaa           = NULL;
    delete prog_godrays;        prog_godrays        = NULL;
    delete prog_tonemapping;    prog_tonemapping    = NULL;
}

bool PostFX::CreateColorFBO(GLuint &fbo, GLuint &tex, int w, int h, GLenum fmt)
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    {
        GLenum type = (fmt == GL_RGBA16F || fmt == GL_RGBA32F) ? GL_FLOAT : GL_UNSIGNED_BYTE;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, GL_RGBA, type, NULL);
    }
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
    if (!GL_VERSION_3_0 && !GL_ARB_framebuffer_object)
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0,
                 GL_RGBA, GL_FLOAT, NULL);
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

    // Attach the G-buffer normal texture as attachment 1 of scene_fbo.
    // Attach the G-buffer albedo texture as attachment 2 of scene_fbo (Phase 3.1b).
    // This makes scene_fbo the MRT FBO: geometry writes scene color to attachment 0,
    // view-space normals to attachment 1, pre-fog diffuse to attachment 2.
    // Default draw buffers are set to [0] only; BindGBuffer() enables [0,1,2].
    glGenTextures(1, &gbuf_norm_tex);
    glBindTexture(GL_TEXTURE_2D, gbuf_norm_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Albedo: RGBA8 (pre-fog diffuse.rgb; alpha unused but pads to 32-bit alignment).
    glGenTextures(1, &gbuf_albedo_tex);
    glBindTexture(GL_TEXTURE_2D, gbuf_albedo_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, scene_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                           GL_TEXTURE_2D, gbuf_norm_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
                           GL_TEXTURE_2D, gbuf_albedo_tex, 0);
    // Restore draw buffers to [0] only — normal geometry path writes one attachment.
    GLenum draw_buf_0 = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &draw_buf_0);

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
    if (!CreateColorFBO(bloom_fbo[0], bloom_tex[0], bw, bh, GL_RGBA16F) ||
        !CreateColorFBO(bloom_fbo[1], bloom_tex[1], bw, bh, GL_RGBA16F))
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

    // --- SSR FBO (Phase 4.1) full resolution RGBA16F ---
    if (!CreateColorFBO(ssr_fbo, ssr_tex, w, h, GL_RGBA16F))
    {
        CONS_Printf("PostFX: SSR FBO failed, SSR disabled.\n");
        ssr_fbo = ssr_tex = 0;
    }

    // --- Volumetric fog FBO (Phase 4.2) full resolution RGBA16F ---
    if (!CreateColorFBO(volfog_fbo, volfog_tex, w, h, GL_RGBA16F))
    {
        CONS_Printf("PostFX: volfog FBO failed, volumetric fog disabled.\n");
        volfog_fbo = volfog_tex = 0;
    }

    // --- LDR intermediate FBO for FXAA (Phase 5.2) full resolution RGBA8 ---
    if (!CreateColorFBO(ldr_fbo, ldr_tex, w, h, GL_RGBA8))
    {
        CONS_Printf("PostFX: LDR FBO failed, FXAA disabled.\n");
        ldr_fbo = ldr_tex = 0;
    }

    // --- God rays FBO (Phase 5.3) full resolution RGBA16F ---
    if (!CreateColorFBO(godrays_fbo, godrays_tex, w, h, GL_RGBA16F))
    {
        CONS_Printf("PostFX: god rays FBO failed, god rays disabled.\n");
        godrays_fbo = godrays_tex = 0;
    }

    CompileShaders();
    BuildSSAOKernel();

    // Screen-quad VAO: 6 vertices (2 triangles) covering texcoord [0,0]-[1,1]
    static const float quad_verts[] = {
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
    };
    glGenVertexArrays(1, &screenquad_vao);
    glGenBuffers(1, &screenquad_vbo);
    glBindVertexArray(screenquad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, screenquad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create a 1×1 transparent null texture for the uVolFog sampler slot.
    // Binding GL name 0 to a sampler gives an incomplete texture which samples as
    // (0,0,0,1) — fog alpha = 1 — blacking out the screen.
    if (!s_null_fog_tex)
    {
        glGenTextures(1, &s_null_fog_tex);
        glBindTexture(GL_TEXTURE_2D, s_null_fog_tex);
        const GLfloat z[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1, 1, 0, GL_RGBA, GL_FLOAT, z);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    fbo_ready = true;

    // Initialize G-buffer (Phase 2.2/3.1/3.1b).
    // scene_fbo is the MRT FBO; gbuf_norm_tex is attachment 1, gbuf_albedo_tex is attachment 2.
    // DeferredRenderer stores refs and creates its independent light_accum_fbo.
    if (g_deferredRenderer)
        g_deferredRenderer->InitGBuffer(scene_fbo, gbuf_norm_tex, gbuf_albedo_tex, scene_depth_tex, w, h);

    CONS_Printf("PostFX: initialized %dx%d.\n", w, h);
    return true;
}

void PostFX::Destroy()
{
    // Destroy G-buffer resources before textures they reference are deleted.
    if (g_deferredRenderer)
        g_deferredRenderer->DestroyGBuffer();

    DeleteShaders();

    if (scene_fbo)         { glDeleteFramebuffers(1, &scene_fbo);      scene_fbo         = 0; }
    if (scene_color_tex)   { glDeleteTextures(1, &scene_color_tex);    scene_color_tex   = 0; }
    if (scene_depth_tex)   { glDeleteTextures(1, &scene_depth_tex);    scene_depth_tex   = 0; }
    if (gbuf_norm_tex)     { glDeleteTextures(1, &gbuf_norm_tex);      gbuf_norm_tex     = 0; }
    if (gbuf_albedo_tex)   { glDeleteTextures(1, &gbuf_albedo_tex);    gbuf_albedo_tex   = 0; }

    for (int i = 0; i < 2; i++)
    {
        if (bloom_fbo[i]) { glDeleteFramebuffers(1, &bloom_fbo[i]); bloom_fbo[i] = 0; }
        if (bloom_tex[i]) { glDeleteTextures(1,    &bloom_tex[i]);  bloom_tex[i] = 0; }
    }

    if (ssao_fbo)      { glDeleteFramebuffers(1, &ssao_fbo);      ssao_fbo      = 0; }
    if (ssao_tex)      { glDeleteTextures(1,     &ssao_tex);       ssao_tex      = 0; }
    if (ssao_blur_fbo) { glDeleteFramebuffers(1, &ssao_blur_fbo);  ssao_blur_fbo = 0; }
    if (ssao_blur_tex) { glDeleteTextures(1,     &ssao_blur_tex);  ssao_blur_tex = 0; }
    if (ssr_fbo)       { glDeleteFramebuffers(1, &ssr_fbo);        ssr_fbo       = 0; }
    if (ssr_tex)       { glDeleteTextures(1,     &ssr_tex);        ssr_tex       = 0; }
    if (volfog_fbo)       { glDeleteFramebuffers(1, &volfog_fbo);      volfog_fbo      = 0; }
    if (volfog_tex)       { glDeleteTextures(1,     &volfog_tex);      volfog_tex      = 0; }
    if (ldr_fbo)          { glDeleteFramebuffers(1, &ldr_fbo);         ldr_fbo         = 0; }
    if (ldr_tex)          { glDeleteTextures(1,     &ldr_tex);         ldr_tex         = 0; }
    if (godrays_fbo)      { glDeleteFramebuffers(1, &godrays_fbo);     godrays_fbo     = 0; }
    if (godrays_tex)      { glDeleteTextures(1,     &godrays_tex);     godrays_tex     = 0; }
    if (s_null_fog_tex)   { glDeleteTextures(1, &s_null_fog_tex);      s_null_fog_tex  = 0; }

    if (screenquad_vao) { glDeleteVertexArrays(1, &screenquad_vao); screenquad_vao = 0; }
    if (screenquad_vbo) { glDeleteBuffers(1,       &screenquad_vbo); screenquad_vbo = 0; }

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
    if (cv_grbloom.value || cv_grssao.value || cv_grssr.value || cv_grvolfog.value
        || cv_grfxaa.value || cv_grgodrays.value) return true;
    // Deferred rendering requires the scene FBO for G-buffer MRT and the
    // final composite pass that reads both scene_color_tex and light_accum_tex.
    if (cv_grdeferred.value && g_deferredRenderer &&
        g_deferredRenderer->IsDeferredLightReady())
        return true;
    return false;
}

void PostFX::BindSceneFBO()
{
    if (fbo_ready && scene_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, scene_fbo);
}

void PostFX::RenderFullscreenQuad()
{
    if (screenquad_vao) {
        glBindVertexArray(screenquad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    } else {
        // Fallback: use gl_MultiTexCoord0-based immediate mode
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
        glEnd();
    }
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
    if (fabsf(proj[0])  > 1e-7f) pi[0]  = 1.0f / proj[0];
    if (fabsf(proj[5])  > 1e-7f) pi[5]  = 1.0f / proj[5];
    if (fabsf(proj[14]) > 1e-7f) pi[11] = 1.0f / proj[14];
    pi[14] = proj[11];                                           // = -1
    if (fabsf(proj[14]) > 1e-7f) pi[15] = proj[10] / proj[14]; // A/B

    // Use G-buffer normals when available (Phase 2.2) — higher quality than depth derivatives.
    GLuint norm_tex = 0;
    if (g_deferredRenderer && g_deferredRenderer->IsGBufferReady() && cv_grdeferred.value)
        norm_tex = (GLuint)g_deferredRenderer->GetNormalTex();
    ShaderProg *ssao_prog = (norm_tex && prog_ssao_gbuf) ? prog_ssao_gbuf : prog_ssao;

    // --- SSAO pass → ssao_tex ---
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    ssao_prog->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_depth_tex);
    GLint l = U(ssao_prog, "uDepth");   if (l >= 0) glUniform1i(l, 0);
    if (norm_tex)
    {
        // Bind G-buffer normal texture to unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, norm_tex);
        l = U(ssao_prog, "uNormal");    if (l >= 0) glUniform1i(l, 1);
        glActiveTexture(GL_TEXTURE0);
    }
    l = U(ssao_prog, "uProjInv");       if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, pi);
    l = U(ssao_prog, "uProj");          if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, proj);
    l = U(ssao_prog, "uKernel");        if (l >= 0) glUniform3fv(l, 16, &ssao_kernel[0][0]);
    l = U(ssao_prog, "uRadius");        if (l >= 0) glUniform1f(l, 64.0f);
    l = U(ssao_prog, "uBias");          if (l >= 0) glUniform1f(l, 0.025f);
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

void PostFX::SSRPass()
{
    if (!ssr_fbo || !prog_ssr) return;
    if (!g_deferredRenderer || !g_deferredRenderer->IsGBufferReady()) return;

    // Fetch the projection matrix and compute its inverse (same as in SSAOPass).
    GLfloat proj[16], pi[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    memset(pi, 0, sizeof(pi));
    if (fabsf(proj[0])  > 1e-7f) pi[0]  = 1.0f / proj[0];
    if (fabsf(proj[5])  > 1e-7f) pi[5]  = 1.0f / proj[5];
    if (fabsf(proj[14]) > 1e-7f) pi[11] = 1.0f / proj[14];
    pi[14] = proj[11];
    if (fabsf(proj[14]) > 1e-7f) pi[15] = proj[10] / proj[14];

    GLuint norm_tex = (GLuint)g_deferredRenderer->GetNormalTex();

    glBindFramebuffer(GL_FRAMEBUFFER, ssr_fbo);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    prog_ssr->Use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_depth_tex);
    GLint l = U(prog_ssr, "uDepth");    if (l >= 0) glUniform1i(l, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, norm_tex);
    l = U(prog_ssr, "uNormal");         if (l >= 0) glUniform1i(l, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, scene_color_tex);
    l = U(prog_ssr, "uScene");          if (l >= 0) glUniform1i(l, 2);

    l = U(prog_ssr, "uProjInv");        if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, pi);
    l = U(prog_ssr, "uProj");           if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, proj);
    l = U(prog_ssr, "uMaxDist");        if (l >= 0) glUniform1f(l, 512.0f);
    l = U(prog_ssr, "uThickness");      if (l >= 0) glUniform1f(l, 16.0f);

    RenderFullscreenQuad();

    ShaderProg::DisableShaders();
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}

void PostFX::VolFogPass()
{
    if (!volfog_fbo || !prog_vol_fog) return;

    // Parse fog color from cv_grfogcolor (hex string "rrggbb").
    unsigned int hexcolor = 0;
    sscanf(cv_grfogcolor.str, "%x", &hexcolor);
    float fr = ((hexcolor >> 16) & 0xFF) / 255.0f;
    float fg = ((hexcolor >>  8) & 0xFF) / 255.0f;
    float fb = ( hexcolor        & 0xFF) / 255.0f;

    // Map density [0,255] to extinction coefficient. At density=100, coeff ≈ 0.002/unit.
    float density = cv_grfogdensity.value * 0.00002f;

    // Compute projection inverse (same analytical inverse used in SSAO/SSR passes).
    GLfloat proj[16], pi[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    memset(pi, 0, sizeof(pi));
    if (fabsf(proj[0])  > 1e-7f) pi[0]  = 1.0f / proj[0];
    if (fabsf(proj[5])  > 1e-7f) pi[5]  = 1.0f / proj[5];
    if (fabsf(proj[14]) > 1e-7f) pi[11] = 1.0f / proj[14];
    pi[14] = proj[11];
    if (fabsf(proj[14]) > 1e-7f) pi[15] = proj[10] / proj[14];

    glBindFramebuffer(GL_FRAMEBUFFER, volfog_fbo);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    prog_vol_fog->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_depth_tex);
    GLint l = U(prog_vol_fog, "uDepth");    if (l >= 0) glUniform1i(l, 0);
    l = U(prog_vol_fog, "uProjInv");        if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, pi);
    l = U(prog_vol_fog, "uFogColor");       if (l >= 0) glUniform3f(l, fr, fg, fb);
    l = U(prog_vol_fog, "uFogDensity");     if (l >= 0) glUniform1f(l, density);
    RenderFullscreenQuad();

    ShaderProg::DisableShaders();
    glActiveTexture(GL_TEXTURE0);
}

void PostFX::GodRaysPass()
{
    if (!godrays_fbo || !prog_godrays) return;

    // --- Project sun world-space direction to screen space ---
    // World direction from azimuth (compass, degrees) and altitude (elevation, degrees).
    // Doom uses Z-up, so: X=east, Y=north, Z=up.
    float az  = cv_grsunazimuth.value  * (3.14159265f / 180.0f);
    float alt = cv_grsunaltitude.value * (3.14159265f / 180.0f);
    float wx = cosf(alt) * sinf(az);
    float wy = cosf(alt) * cosf(az);
    float wz = sinf(alt);

    // Transform world direction to eye space (use modelview rotation only — no translation).
    GLfloat mv[16], proj[16];
    glGetFloatv(GL_MODELVIEW_MATRIX,  mv);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

    float ex = mv[0]*wx + mv[4]*wy + mv[8]*wz;
    float ey = mv[1]*wx + mv[5]*wy + mv[9]*wz;
    float ez = mv[2]*wx + mv[6]*wy + mv[10]*wz;

    // Project eye-space direction to clip space (standard GL perspective).
    // clipw = proj[11]*ez (= -ez for standard perspective where proj[11]=-1).
    float clipw = proj[3]*ex + proj[7]*ey + proj[11]*ez + proj[15];
    if (clipw <= 0.001f)
    {
        // Sun is behind the camera — clear god rays buffer and bail.
        glBindFramebuffer(GL_FRAMEBUFFER, godrays_fbo);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    float clipx = proj[0]*ex + proj[4]*ey + proj[8]*ez  + proj[12];
    float clipy = proj[1]*ex + proj[5]*ey + proj[9]*ez  + proj[13];
    float sx = (clipx / clipw) * 0.5f + 0.5f;
    float sy = (clipy / clipw) * 0.5f + 0.5f;

    // --- Render radial-blur god rays into godrays_fbo ---
    glBindFramebuffer(GL_FRAMEBUFFER, godrays_fbo);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    prog_godrays->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_color_tex);
    GLint l = U(prog_godrays, "uScene");    if (l >= 0) glUniform1i(l, 0);
    l = U(prog_godrays, "uLightPos");       if (l >= 0) glUniform2f(l, sx, sy);
    l = U(prog_godrays, "uDensity");        if (l >= 0) glUniform1f(l, 0.9f);
    l = U(prog_godrays, "uDecay");          if (l >= 0) glUniform1f(l, 0.96f);
    l = U(prog_godrays, "uWeight");         if (l >= 0) glUniform1f(l, 0.012f);
    l = U(prog_godrays, "uExposure");       if (l >= 0) glUniform1f(l, 1.0f);
    RenderFullscreenQuad();

    ShaderProg::DisableShaders();
    glActiveTexture(GL_TEXTURE0);
}

void PostFX::FXAAPass()
{
    if (!prog_fxaa || !ldr_tex) return;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    prog_fxaa->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ldr_tex);
    GLint l = U(prog_fxaa, "uScene");     if (l >= 0) glUniform1i(l, 0);
    l = U(prog_fxaa, "uTexelSize");       if (l >= 0) glUniform2f(l, 1.0f / width, 1.0f / height);
    RenderFullscreenQuad();

    ShaderProg::DisableShaders();
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

    // --- Composite scene + bloom → default framebuffer (or LDR FBO if FXAA follows) ---
    GLuint out_fbo = (cv_grfxaa.value && ldr_fbo) ? ldr_fbo : 0;
    glBindFramebuffer(GL_FRAMEBUFFER, out_fbo);
    glViewport(0, 0, width, height);

    prog_composite->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src);
    l = U(prog_composite, "uScene");          if (l >= 0) glUniform1i(l, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloom_tex[0]);
    l = U(prog_composite, "uBloom");          if (l >= 0) glUniform1i(l, 1);
    // Bind light accumulation texture to unit 2 (black when deferred is off).
    GLuint lat = (g_deferredRenderer && cv_grdeferred.value)
                 ? (GLuint)g_deferredRenderer->GetLightAccumTex() : 0;
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, lat);
    l = U(prog_composite, "uLightAccum");     if (l >= 0) glUniform1i(l, 2);
    // Bind SSR texture to unit 3 (Phase 4.1; black when SSR disabled).
    GLuint ssrt = (cv_grssr.value && ssr_tex) ? ssr_tex : 0;
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ssrt);
    l = U(prog_composite, "uSSR");            if (l >= 0) glUniform1i(l, 3);
    float ssrStr = (cv_grssr.value && ssr_tex) ? cv_grssrstrength.value / 100.0f : 0.0f;
    l = U(prog_composite, "uSSRStrength");    if (l >= 0) glUniform1f(l, ssrStr);
    // Bind volumetric fog texture to unit 4 (Phase 4.2; transparent when disabled).
    GLuint vft = (cv_grvolfog.value && volfog_tex) ? volfog_tex : s_null_fog_tex;
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, vft);
    l = U(prog_composite, "uVolFog");         if (l >= 0) glUniform1i(l, 4);
    // Bind god rays texture to unit 5 (Phase 5.3); additive blend is safe with tex 0.
    GLuint grt = (cv_grgodrays.value && godrays_tex) ? godrays_tex : 0;
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, grt);
    l = U(prog_composite, "uGodRays");           if (l >= 0) glUniform1i(l, 5);
    float grStr = (cv_grgodrays.value && godrays_tex) ? cv_grgodraysstrength.value : 0.0f;
    l = U(prog_composite, "uGodRaysStrength");   if (l >= 0) glUniform1f(l, grStr);
    glActiveTexture(GL_TEXTURE0);
    l = U(prog_composite, "uBloomIntensity"); if (l >= 0) glUniform1f(l, 1.0f);
    RenderFullscreenQuad();

    ShaderProg::DisableShaders();
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}

void PostFX::ApplyEffects()
{
    if (!fbo_ready) return;

    // Check for any GL errors that arrived before we start the PostFX composite.
    // Gated under devparm: during the loading-screen phase certain GL operations
    // (e.g. texture uploads triggered by the first DrawConsole call) can leave a
    // spurious GL_INVALID_VALUE in the queue before the first real rendered frame.
    // These are harmless init artifacts; only report them in developer mode.
    if (devparm)
    {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
            CONS_Printf("PostFX::ApplyEffects entry GL error: 0x%04x\n", err);
    }

    // Verify the scene FBO is still intact before sampling it (devparm only).
    if (devparm)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, scene_fbo);
        GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (st != GL_FRAMEBUFFER_COMPLETE)
            CONS_Printf("PostFX::ApplyEffects: scene_fbo (id=%u) INCOMPLETE: 0x%04x\n",
                        scene_fbo, st);
    }

    // Unbind scene FBO — subsequent rendering goes to the default framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Save relevant GL state explicitly (replaces glPushAttrib).
    GLboolean was_depth, was_cull, was_alpha, was_blend, was_fog, was_tex2d;
    GLint saved_vp[4];
    glGetBooleanv(GL_DEPTH_TEST,  &was_depth);
    glGetBooleanv(GL_CULL_FACE,   &was_cull);
    glGetBooleanv(GL_ALPHA_TEST,  &was_alpha);
    glGetBooleanv(GL_BLEND,       &was_blend);
    glGetBooleanv(GL_FOG,         &was_fog);
    glGetBooleanv(GL_TEXTURE_2D,  &was_tex2d);
    glGetIntegerv(GL_VIEWPORT,    saved_vp);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_FOG);
    glEnable(GL_TEXTURE_2D);

    // --- SSAO (must run before bloom so bloom picks up the AO-darkened scene) ---
    if (cv_grssao.value && ssao_fbo)
        SSAOPass();

    // --- SSR (Phase 4.1, runs after SSAO so it samples the AO-corrected scene) ---
    if (cv_grssr.value && ssr_fbo)
        SSRPass();

    // --- Volumetric fog (Phase 4.2) ---
    if (cv_grvolfog.value && volfog_fbo)
        VolFogPass();

    // --- God rays (Phase 5.3): radial-blur light shafts from projected sun position ---
    if (cv_grgodrays.value && godrays_fbo)
        GodRaysPass();

    // --- Bloom ---
    if (cv_grbloom.value)
    {
        BloomPass();
    }
    else
    {
        // No bloom — tone-map the (possibly SSAO-composited) scene directly to screen
        // (or to the LDR FBO when FXAA follows).
        GLuint src = (cv_grssao.value && ssao_fbo && ssao_tex)
                     ? ssao_tex : scene_color_tex;
        GLuint out_fbo = (cv_grfxaa.value && ldr_fbo) ? ldr_fbo : 0;
        glBindFramebuffer(GL_FRAMEBUFFER, out_fbo);
        glViewport(0, 0, width, height);
        if (prog_tonemapping) {
            prog_tonemapping->Use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, src);
            GLint l = U(prog_tonemapping, "uHDR"); if (l >= 0) glUniform1i(l, 0);
            // Bind light accumulation texture to unit 1 (black when deferred is off).
            GLuint lat = (g_deferredRenderer && cv_grdeferred.value)
                         ? (GLuint)g_deferredRenderer->GetLightAccumTex() : 0;
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, lat);
            l = U(prog_tonemapping, "uLightAccum"); if (l >= 0) glUniform1i(l, 1);
            // Bind SSR texture to unit 2 (Phase 4.1; black when SSR disabled).
            GLuint ssrt = (cv_grssr.value && ssr_tex) ? ssr_tex : 0;
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, ssrt);
            l = U(prog_tonemapping, "uSSR");        if (l >= 0) glUniform1i(l, 2);
            float ssrStr = (cv_grssr.value && ssr_tex) ? cv_grssrstrength.value / 100.0f : 0.0f;
            l = U(prog_tonemapping, "uSSRStrength"); if (l >= 0) glUniform1f(l, ssrStr);
            // Bind volumetric fog texture to unit 3 (Phase 4.2; transparent when disabled).
            GLuint vft = (cv_grvolfog.value && volfog_tex) ? volfog_tex : s_null_fog_tex;
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, vft);
            l = U(prog_tonemapping, "uVolFog");          if (l >= 0) glUniform1i(l, 3);
            // Bind god rays texture to unit 4 (Phase 5.3); additive, safe with tex 0.
            GLuint grt = (cv_grgodrays.value && godrays_tex) ? godrays_tex : 0;
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, grt);
            l = U(prog_tonemapping, "uGodRays");         if (l >= 0) glUniform1i(l, 4);
            float grStr = (cv_grgodrays.value && godrays_tex) ? cv_grgodraysstrength.value : 0.0f;
            l = U(prog_tonemapping, "uGodRaysStrength"); if (l >= 0) glUniform1f(l, grStr);
            glActiveTexture(GL_TEXTURE0);
            RenderFullscreenQuad();
            ShaderProg::DisableShaders();
        } else {
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
    }

    // --- FXAA (Phase 5.2): anti-alias the LDR output → default framebuffer ---
    if (cv_grfxaa.value && ldr_fbo)
        FXAAPass();

    ShaderProg::DisableShaders();
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);

    // Restore GL state (replaces glPopAttrib).
    if (was_depth)  glEnable(GL_DEPTH_TEST);  else glDisable(GL_DEPTH_TEST);
    if (was_cull)   glEnable(GL_CULL_FACE);   else glDisable(GL_CULL_FACE);
    if (was_alpha)  glEnable(GL_ALPHA_TEST);  else glDisable(GL_ALPHA_TEST);
    if (was_blend)  glEnable(GL_BLEND);       else glDisable(GL_BLEND);
    if (was_fog)    glEnable(GL_FOG);         else glDisable(GL_FOG);
    if (was_tex2d)  glEnable(GL_TEXTURE_2D);  else glDisable(GL_TEXTURE_2D);
    glViewport(saved_vp[0], saved_vp[1], saved_vp[2], saved_vp[3]);
}
