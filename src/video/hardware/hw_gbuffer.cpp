//-----------------------------------------------------------------------------
//
// Deferred rendering G-Buffer implementation for Doom Legacy
//
// Phase 2.2: G-buffer MRT FBO — normals written alongside scene color so that
//            the SSAO pass can sample real normals instead of depth derivatives.
// Phase 3.1: Deferred dynamic light accumulation — one fullscreen-triangle pass
//            per dynamic light, additively blending into the scene color buffer.
//
//-----------------------------------------------------------------------------

// Include GLEW first to avoid conflicts
#include <GL/glew.h>

#include "hw_gbuffer.h"
#include "oglrenderer.hpp"
#include "oglshaders.h"
#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include <cstdio>  // snprintf

// Global instance
DeferredRenderer* g_deferredRenderer = nullptr;

// Frame counter (bumped by BindGBuffer — one call per rendered frame).
static int s_frame = 0;

// ---------------------------------------------------------------------------
// Diagnostic helpers
// ---------------------------------------------------------------------------

/// Drain the GL error queue, printing each error.  Returns true if any error
/// was found.  'where' identifies the call site in the log output.
static bool GBufCheckErrors(const char *where)
{
    GLenum err;
    bool had = false;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        CONS_Printf("GBuffer[%05d] GL error at %s: 0x%04x\n", s_frame, where, err);
        had = true;
    }
    return had;
}

/// Check and log the completeness status of an FBO.  Returns true if complete.
static bool GBufCheckFBO(GLuint fbo, const char *name)
{
    if (!fbo)
    {
        CONS_Printf("GBuffer[%05d] %s is 0 (null FBO)\n", s_frame, name);
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE)
    {
        CONS_Printf("GBuffer[%05d] %s (id=%u) INCOMPLETE: 0x%04x\n",
                    s_frame, name, fbo, st);
        return false;
    }
    return true;
}

/// Print a one-line periodic status dump (called every LOG_INTERVAL frames).
void DeferredRenderer::LogStatus() const
{
    GLint cur_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &cur_fbo);
    CONS_Printf("GBuffer[%05d] ready=%d passOK=%d"
                " scene_fbo=%u accum_fbo=%u accum_tex=%u"
                " norm_tex=%u albedo_tex=%u depth_tex=%u cur_fbo=%d\n",
                s_frame,
                (int)gbuf_ready, (int)(gbuf_ready && deferred_ready && light_accum_fbo != 0),
                stored_scene_fbo, light_accum_fbo, light_accum_tex,
                stored_gbuf_norm_tex, stored_gbuf_albedo_tex, stored_scene_depth_tex,
                cur_fbo);
}

// ---------------------------------------------------------------------------
// Deferred light accumulation shaders (Phase 3.1)
// ---------------------------------------------------------------------------

static const char *deferred_vert_src =
    "#version 330 compatibility\n"
    "out vec2 v_uv;\n"
    "void main() {\n"
    // Fullscreen triangle: 3 vertices from gl_VertexID cover the entire NDC cube.
    //   vtx 0: (-1,-1) uv (0,0)
    //   vtx 1: ( 3,-1) uv (2,0)
    //   vtx 2: (-1, 3) uv (0,2)
    // The triangle is large enough that the rasterizer clips it to [-1,1]x[-1,1].
    "    float x = float(gl_VertexID == 1) * 4.0 - 1.0;\n"
    "    float y = float(gl_VertexID == 2) * 4.0 - 1.0;\n"
    "    v_uv = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);\n"
    "    gl_Position = vec4(x, y, 0.0, 1.0);\n"
    "}\n";

static const char *deferred_frag_src =
    "#version 330 compatibility\n"
    "in vec2 v_uv;\n"
    "layout(location=0) out vec4 fragColor;\n"
    // G-buffer textures bound by BeginLightingPass:
    //   unit 0 = depth, unit 1 = normal (packed [0,1]), unit 2 = albedo (Phase 3.1b)
    "uniform sampler2D uDepth;\n"
    "uniform sampler2D uNormal;\n"
    "uniform sampler2D uAlbedo;\n"  // pre-fog diffuse.rgb (RGBA8)
    // Perspective reconstruction parameters (from cached_proj):
    //   projA  = cached_proj[10]  ( (f+n)/(n-f) )
    //   projB  = cached_proj[14]  ( 2*f*n/(n-f) )
    //   projSX = cached_proj[0]   ( f/aspect )
    //   projSY = cached_proj[5]   ( f )
    "uniform float uProjA;\n"
    "uniform float uProjB;\n"
    "uniform float uProjSX;\n"
    "uniform float uProjSY;\n"
    // Per-light uniforms uploaded by RenderLight():
    "uniform vec4  uLightEye;\n"         // xyz = eye-space position, w = attenuation radius
    "uniform vec3  uLightColor;\n"       // RGB in [0,1]
    // Phase 3.2: omnidirectional cube shadow map
    //   uShadowCube: samplerCube on unit 3 storing normalized linear distance
    //   uShadowCubeActive: 1 when a valid cube shadow map is bound for this light
    //   uInvView: mat3 = transpose of view rotation (converts eye-space dir to world-space)
    "uniform samplerCube uShadowCube;\n"
    "uniform int  uShadowCubeActive;\n"
    "uniform mat3 uInvView;\n"
    // Phase 5.1: Blinn-Phong specular controls.
    "uniform float uSpecStrength;\n"  // per-light specular weight [0,1]
    "uniform float uSpecExp;\n"       // Phong exponent [4,128]
    "void main() {\n"
    // Discard background fragments (depth == 1.0 means sky / no geometry).
    "    float depth = texture(uDepth, v_uv).r;\n"
    "    if (depth >= 1.0) discard;\n"
    // Reconstruct eye-space position from window-space depth.
    //   z_ndc in [-1,1]: z_ndc = depth*2-1
    //   eye-space z:     z_eye = -projB / (z_ndc + projA)
    "    float z_ndc = depth * 2.0 - 1.0;\n"
    "    float z_eye = -uProjB / (z_ndc + uProjA);\n"
    "    vec2  xy_ndc = v_uv * 2.0 - 1.0;\n"
    "    float x_eye  = xy_ndc.x * (-z_eye) / uProjSX;\n"
    "    float y_eye  = xy_ndc.y * (-z_eye) / uProjSY;\n"
    "    vec3 fragPos = vec3(x_eye, y_eye, z_eye);\n"
    // Unpack view-space normal from [0,1] storage.
    "    vec3 N = normalize(texture(uNormal, v_uv).rgb * 2.0 - 1.0);\n"
    // Point light: quadratic attenuation, cosine NdotL.
    "    vec3  Ldir  = uLightEye.xyz - fragPos;\n"
    "    float dist  = length(Ldir);\n"
    "    float atten = clamp(1.0 - dist / uLightEye.w, 0.0, 1.0);\n"
    "    atten *= atten;\n"
    "    float NdotL = max(dot(N, normalize(Ldir)), 0.0);\n"
    // Phase 3.1b: multiply by pre-fog surface albedo for correct surface tinting.
    "    vec3  albedo = texture(uAlbedo, v_uv).rgb;\n"
    // Phase 3.2: omnidirectional cube shadow test.
    // eyeVecToFrag: eye-space vector from light to fragment.
    // worldVecToFrag: same direction in world space (for cube map face selection).
    // storedDist: normalized linear distance [0,1] read from cube map.
    // thisDist:   normalized distance of this fragment from the light.
    // A bias of 0.01 prevents self-shadowing artefacts.
    // Shadowed fragments receive 0.3 ambient (matches the 2D shadow map floor).
    "    float shadow = 1.0;\n"
    "    if (uShadowCubeActive != 0) {\n"
    "        vec3  eyeVecToFrag   = fragPos - uLightEye.xyz;\n"
    "        vec3  worldVecToFrag = uInvView * eyeVecToFrag;\n"
    "        float storedDist     = texture(uShadowCube, worldVecToFrag).r;\n"
    "        float thisDist       = length(eyeVecToFrag) / uLightEye.w;\n"
    "        shadow = (thisDist - 0.01 > storedDist) ? 0.3 : 1.0;\n"
    "    }\n"
    // Phase 5.1: Blinn-Phong specular.
    // V = view direction (eye is at origin in view space), H = half-vector.
    // Specular is gated by NdotL so back-facing surfaces receive no highlight.
    // Specular color inherits the surface albedo so highlights are surface-tinted.
    "    vec3  V     = normalize(-fragPos);\n"
    "    vec3  H     = normalize(normalize(Ldir) + V);\n"
    "    float NdotH = max(dot(N, H), 0.0);\n"
    "    float spec  = pow(NdotH, uSpecExp) * uSpecStrength * NdotL;\n"
    "    fragColor = vec4(albedo * uLightColor * (NdotL + spec) * atten * shadow, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Helper: compile a single shader stage
// ---------------------------------------------------------------------------

GLuint DeferredRenderer::CompileShader(GLenum type, const char *src)
{
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);

    GLint ok = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        if (len > 0)
        {
            char *log = new char[len];
            glGetShaderInfoLog(id, len, NULL, log);
            CONS_Printf("DeferredRenderer: shader compile error: %s\n", log);
            delete[] log;
        }
        glDeleteShader(id);
        return 0;
    }
    return id;
}

// ---------------------------------------------------------------------------
// DeferredRenderer constructor / destructor
// ---------------------------------------------------------------------------

DeferredRenderer::DeferredRenderer()
    : useDeferred(false), maxLightsPerSurface(32),
      bloomEnabled(true), bloomIntensity(0.5f), fogDensity(0.001f),
      stored_scene_fbo(0), stored_gbuf_norm_tex(0), stored_gbuf_albedo_tex(0), stored_scene_depth_tex(0),
      gbuf_width(0), gbuf_height(0), gbuf_ready(false),
      light_accum_tex(0), light_accum_fbo(0),
      deferred_prog(0), deferred_vao(0), null_cube_tex(0), deferred_ready(false)
{
    fogColor[0] = fogColor[1] = fogColor[2] = 0.0f;
    memset(&dloc, -1, sizeof(dloc));
}

DeferredRenderer::~DeferredRenderer()
{
    Destroy();
}

// ---------------------------------------------------------------------------
// Init / Destroy
// ---------------------------------------------------------------------------

bool DeferredRenderer::Init(int width, int height)
{
    useDeferred = false;
    bloomEnabled = true;
    bloomIntensity = 0.5f;
    fogDensity = 0.001f;
    maxLightsPerSurface = 32;

    InitDeferredLightPass();

    CONS_Printf("Deferred renderer initialized (Phase 3.1 light pass %s).\n",
                deferred_ready ? "ready" : "unavailable");
    return true;
}

void DeferredRenderer::Destroy()
{
    DestroyGBuffer();
    DestroyDeferredLightPass();
}

// ---------------------------------------------------------------------------
// G-buffer MRT FBO (Phase 2.2 / 3.1 architectural fix)
//
// The scene_fbo from PostFX is the MRT FBO (attachment 0 = scene color,
// attachment 1 = gbuf_norm_tex).  DeferredRenderer no longer owns a separate
// gbuf_fbo — it just sets draw buffers on the already-bound scene_fbo.
//
// The light accumulation FBO (light_accum_fbo) uses a completely independent
// texture (light_accum_tex).  No texture is shared between adjacent FBOs,
// which guarantees the driver flushes its render-target cache on every
// FBO transition.  This eliminates the 30-second glitch caused by the
// stale render-cache read when gbuf_fbo and scene_fbo shared scene_color_tex.
// ---------------------------------------------------------------------------

bool DeferredRenderer::InitGBuffer(unsigned int scene_fbo,
                                   unsigned int gbuf_norm_tex,
                                   unsigned int gbuf_albedo_tex,
                                   unsigned int scene_depth_tex, int w, int h)
{
    DestroyGBuffer();

    // Store refs (NOT owned — PostFX owns and deletes these).
    stored_scene_fbo         = (GLuint)scene_fbo;
    stored_gbuf_norm_tex     = (GLuint)gbuf_norm_tex;
    stored_gbuf_albedo_tex   = (GLuint)gbuf_albedo_tex;
    stored_scene_depth_tex   = (GLuint)scene_depth_tex;
    gbuf_width  = w;
    gbuf_height = h;

    // scene_fbo already has attachment 0 (color), attachment 1 (normals), and
    // attachment 2 (albedo, Phase 3.1b) set up by PostFX::Init().
    // Verify the MRT FBO is complete before proceeding.
    glBindFramebuffer(GL_FRAMEBUFFER, stored_scene_fbo);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        CONS_Printf("GBuffer: scene MRT FBO incomplete (0x%x), deferred normals disabled.\n",
                    status);
        stored_scene_fbo = stored_gbuf_norm_tex = stored_scene_depth_tex = 0;
        return false;
    }

    gbuf_ready = true;

    // Create the light accumulation FBO — RGBA16F, same size as scene.
    // This FBO shares NO textures with scene_fbo, so the driver must flush
    // its render-target cache on every transition between the two FBOs.
    glGenTextures(1, &light_accum_tex);
    glBindTexture(GL_TEXTURE_2D, light_accum_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &light_accum_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, light_accum_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, light_accum_tex, 0);
    // No depth attachment — we only write RGB accumulation here.
    GLenum la_buf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &la_buf);

    GLenum la_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (la_status != GL_FRAMEBUFFER_COMPLETE)
    {
        CONS_Printf("GBuffer: light_accum FBO incomplete (0x%x).\n"
                    "  Deferred light pass disabled; ambient floor + SSAO still active.\n",
                    la_status);
        glDeleteFramebuffers(1, &light_accum_fbo); light_accum_fbo = 0;
        glDeleteTextures(1,     &light_accum_tex);  light_accum_tex = 0;
        // gbuf_ready remains true — normals/SSAO still work via stored_gbuf_norm_tex.
        // IsDeferredPassSafe() returns false (light_accum_fbo == 0).
    }

    CONS_Printf("GBuffer: %dx%d MRT ready — light_accum_fbo %s.\n",
                w, h, light_accum_fbo ? "OK" : "unavailable");
    return true;
}

void DeferredRenderer::BindGBuffer()
{
    if (!gbuf_ready) return;

    ++s_frame;

    // Periodic diagnostic dump every 300 frames (~5 s at 60 fps).
    static const int LOG_INTERVAL = 300;
    if (s_frame % LOG_INTERVAL == 0)
        LogStatus();

    // Error check: any leftover GL errors from previous frame's rendering?
    GBufCheckErrors("BindGBuffer-entry");

    // scene_fbo is already bound by PostFX::BindSceneFBO().
    // Enable all three MRT attachments: color + normals + albedo (Phase 3.1b).
    GLenum bufs[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, bufs);

    // Clear light accumulation from the previous frame so that frames with no
    // dynamic lights contribute zero rather than leftover data.
    if (light_accum_fbo)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, light_accum_fbo);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // Return to scene_fbo in MRT mode.
        glBindFramebuffer(GL_FRAMEBUFFER, stored_scene_fbo);
        glDrawBuffers(3, bufs);
    }
}

void DeferredRenderer::DestroyGBuffer()
{
    if (light_accum_fbo) { glDeleteFramebuffers(1, &light_accum_fbo); light_accum_fbo = 0; }
    if (light_accum_tex) { glDeleteTextures(1,     &light_accum_tex);  light_accum_tex = 0; }
    // Stored refs are owned by PostFX — do NOT delete them here.
    stored_scene_fbo = stored_gbuf_norm_tex = stored_gbuf_albedo_tex = stored_scene_depth_tex = 0;
    gbuf_width = gbuf_height = 0;
    gbuf_ready = false;
}

// ---------------------------------------------------------------------------
// Phase 3.1: deferred dynamic light accumulation pass
// ---------------------------------------------------------------------------

bool DeferredRenderer::InitDeferredLightPass()
{
    DestroyDeferredLightPass();

    GLuint vs = CompileShader(GL_VERTEX_SHADER,   deferred_vert_src);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, deferred_frag_src);
    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    deferred_prog = glCreateProgram();
    glAttachShader(deferred_prog, vs);
    glAttachShader(deferred_prog, fs);
    glLinkProgram(deferred_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(deferred_prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetProgramiv(deferred_prog, GL_INFO_LOG_LENGTH, &len);
        if (len > 0)
        {
            char *log = new char[len];
            glGetProgramInfoLog(deferred_prog, len, NULL, log);
            CONS_Printf("DeferredRenderer: program link error: %s\n", log);
            delete[] log;
        }
        glDeleteProgram(deferred_prog);
        deferred_prog = 0;
        return false;
    }

    // Resolve uniform locations
    glUseProgram(deferred_prog);
    dloc.uDepth  = glGetUniformLocation(deferred_prog, "uDepth");
    dloc.uNormal = glGetUniformLocation(deferred_prog, "uNormal");
    dloc.uAlbedo = glGetUniformLocation(deferred_prog, "uAlbedo");
    dloc.uProjA  = glGetUniformLocation(deferred_prog, "uProjA");
    dloc.uProjB  = glGetUniformLocation(deferred_prog, "uProjB");
    dloc.uProjSX = glGetUniformLocation(deferred_prog, "uProjSX");
    dloc.uProjSY = glGetUniformLocation(deferred_prog, "uProjSY");
    dloc.uLightEye   = glGetUniformLocation(deferred_prog, "uLightEye");
    dloc.uLightColor = glGetUniformLocation(deferred_prog, "uLightColor");

    dloc.uShadowCube       = glGetUniformLocation(deferred_prog, "uShadowCube");
    dloc.uShadowCubeActive = glGetUniformLocation(deferred_prog, "uShadowCubeActive");
    dloc.uInvView          = glGetUniformLocation(deferred_prog, "uInvView");
    dloc.uSpecStrength     = glGetUniformLocation(deferred_prog, "uSpecStrength");
    dloc.uSpecExp          = glGetUniformLocation(deferred_prog, "uSpecExp");

    // Bind samplers to fixed texture units (depth=0, normals=1, albedo=2, shadowCube=3)
    if (dloc.uDepth      >= 0) glUniform1i(dloc.uDepth,      0);
    if (dloc.uNormal     >= 0) glUniform1i(dloc.uNormal,     1);
    if (dloc.uAlbedo     >= 0) glUniform1i(dloc.uAlbedo,     2);
    if (dloc.uShadowCube >= 0) glUniform1i(dloc.uShadowCube, 3);
    glUseProgram(0);

    // Create 1×1 all-ones R32F cube map used when no shadow is active.
    // Binding texture 0 to samplerCube is undefined behaviour; this prevents it.
    glGenTextures(1, &null_cube_tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, null_cube_tex);
    static const GLfloat white = 1.0f;
    for (int f = 0; f < 6; f++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_R32F,
                     1, 1, 0, GL_RED, GL_FLOAT, &white);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Empty VAO for the fullscreen-triangle draw (no vertex attributes needed)
    glGenVertexArrays(1, &deferred_vao);

    deferred_ready = true;
    return true;
}

void DeferredRenderer::DestroyDeferredLightPass()
{
    if (deferred_prog)  { glDeleteProgram(deferred_prog); deferred_prog = 0; }
    if (deferred_vao)   { glDeleteVertexArrays(1, &deferred_vao); deferred_vao = 0; }
    if (null_cube_tex)  { glDeleteTextures(1, &null_cube_tex); null_cube_tex = 0; }
    deferred_ready = false;
}

void DeferredRenderer::BeginLightingPass(GLuint depth_tex, GLuint normal_tex,
                                         GLfloat projA, GLfloat projB,
                                         GLfloat projSX, GLfloat projSY,
                                         const GLfloat *invView)
{
    if (!deferred_ready)
        return;

    // Drain any pre-existing GL errors so we get a clean baseline.
    GBufCheckErrors("BeginLightingPass-entry");

    // Verify light_accum_fbo is still complete (should never fail, but check anyway).
    if (!GBufCheckFBO(light_accum_fbo, "light_accum_fbo"))
    {
        CONS_Printf("GBuffer[%05d] BeginLightingPass: light_accum_fbo invalid — "
                    "aborting pass (lights will be invisible this frame)\n", s_frame);
        // Restore scene_fbo so rendering continues correctly.
        if (stored_scene_fbo)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, stored_scene_fbo);
            GLenum buf0 = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf0);
        }
        return;
    }
    // Rebind stored_scene_fbo (GBufCheckFBO left light_accum_fbo bound).
    if (stored_scene_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, stored_scene_fbo);

    // Reset scene_fbo draw buffers to attachment 0 only — end the MRT geometry phase.
    // BindGBuffer() set them to [0,1]; we must restore [0] before binding
    // light_accum_fbo so that any subsequent scene_fbo bind (e.g. in EndLightingPass)
    // only writes to the colour attachment, not the normal attachment.
    if (stored_scene_fbo)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, stored_scene_fbo);
        GLenum buf0 = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &buf0);
    }

    // Bind the independent light accumulation FBO.
    glBindFramebuffer(GL_FRAMEBUFFER, light_accum_fbo);
    GBufCheckErrors("BeginLightingPass-after-bind");

    // Additive blend: each light adds its contribution.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // No depth writes or depth test for screen-space light quads.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glUseProgram(deferred_prog);

    // Bind G-buffer textures (depth=unit 0, normals=unit 1, albedo=unit 2)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth_tex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_tex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, stored_gbuf_albedo_tex);

    // Pre-bind the null cube map to unit 3; RenderLight() will swap in the real one.
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, null_cube_tex);

    glActiveTexture(GL_TEXTURE0);

    // Upload projection reconstruction parameters
    if (dloc.uProjA  >= 0) glUniform1f(dloc.uProjA,  projA);
    if (dloc.uProjB  >= 0) glUniform1f(dloc.uProjB,  projB);
    if (dloc.uProjSX >= 0) glUniform1f(dloc.uProjSX, projSX);
    if (dloc.uProjSY >= 0) glUniform1f(dloc.uProjSY, projSY);

    // Upload inverse view rotation matrix (eye-space → world-space directions).
    // Needed by the cube shadow sampling in the deferred shader.
    if (invView && dloc.uInvView >= 0)
        glUniformMatrix3fv(dloc.uInvView, 1, GL_FALSE, invView);

    // Phase 5.1: Blinn-Phong specular parameters.
    if (dloc.uSpecStrength >= 0) glUniform1f(dloc.uSpecStrength, cv_grspecular.value);
    if (dloc.uSpecExp      >= 0) glUniform1f(dloc.uSpecExp,      (float)cv_grspecularexp.value);

    // Start with shadow inactive; RenderLight() enables it per-light.
    if (dloc.uShadowCubeActive >= 0) glUniform1i(dloc.uShadowCubeActive, 0);

    glBindVertexArray(deferred_vao);
}

void DeferredRenderer::RenderLight(float ex, float ey, float ez,
                                   float radius, float r, float g, float b,
                                   GLuint shadow_cube_tex)
{
    if (!deferred_ready || !deferred_prog)
        return;

    if (dloc.uLightEye >= 0)
    {
        GLfloat v[4] = { ex, ey, ez, radius };
        glUniform4fv(dloc.uLightEye, 1, v);
    }
    if (dloc.uLightColor >= 0)
    {
        GLfloat c[3] = { r, g, b };
        glUniform3fv(dloc.uLightColor, 1, c);
    }

    // Phase 3.2: bind cube shadow map (or null cube) to unit 3.
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
                  shadow_cube_tex ? shadow_cube_tex : null_cube_tex);
    if (dloc.uShadowCubeActive >= 0)
        glUniform1i(dloc.uShadowCubeActive, shadow_cube_tex ? 1 : 0);
    glActiveTexture(GL_TEXTURE0);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void DeferredRenderer::EndLightingPass()
{
    if (!deferred_ready)
        return;

    glBindVertexArray(0);
    glUseProgram(0);

    // Restore standard alpha-blend state before disabling blend.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    // Unbind deferred textures from their units.
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Restore scene_fbo with draw buffers [0] so subsequent HUD/sprite rendering
    // and PostFX reads go to the correct attachment.
    if (stored_scene_fbo)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, stored_scene_fbo);
        GLenum buf0 = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &buf0);
    }
    else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GBufCheckErrors("EndLightingPass-exit");
}

// ---------------------------------------------------------------------------
// Geometry pass stubs (reserved for future full-deferred geometry path)
// ---------------------------------------------------------------------------

void DeferredRenderer::BeginGeometryPass()
{
    // Phase 3.1 uses the existing forward geometry pass + G-buffer normal output.
    // A future phase could bind the G-buffer here and strip material-level lighting.
}

void DeferredRenderer::EndGeometryPass()
{
}

// ---------------------------------------------------------------------------
// Post-processing stubs
// ---------------------------------------------------------------------------

void DeferredRenderer::BeginPostProcess() {}
void DeferredRenderer::EndPostProcess()   {}

void DeferredRenderer::ApplyBloom()
{
    // Bloom is handled by PostFX (hw_postfx.cpp).
}

void DeferredRenderer::ApplyFog(float /*viewZ*/)
{
    // Per-sector fog handled by the forward shader; deferred fog reserved for a future pass.
}
