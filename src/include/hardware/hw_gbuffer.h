//-----------------------------------------------------------------------------
//
// Deferred rendering G-Buffer for Doom Legacy
//
// G-Buffer contains:
// - Position buffer (RGBA32F: xyz + depth)
// - Normal buffer   (RGBA16F: normal xyz + material ID)   — attachment 1
// - Albedo buffer   (RGBA8: pre-fog diffuse.rgb)          — attachment 2 (Phase 3.1b)
// - Depth buffer (from default depth)
//
//-----------------------------------------------------------------------------

#ifndef hw_gbuffer_h
#define hw_gbuffer_h

#include "doomdef.h"

// Forward declarations
class OGLRenderer;

// Light types (matching GZDoom)
enum LightType {
    LIGHT_POINT = 0,
    LIGHT_PULSE,
    LIGHT_FLICKER,
    LIGHT_RANDOM,
    LIGHT_SECTOR,
    LIGHT_COLORPULSE,
    LIGHT_COLORFLICKER,
    LIGHT_SPOT
};

// Light flags (matching GZDoom)
enum LightFlags {
    LF_SUBTRACTIVE   = 0x01,
    LF_ADDITIVE      = 0x02,
    LF_DONTLIGHTSELF = 0x04,
    LF_ATTENUATE     = 0x08,
    LF_NOSHADOWMAP   = 0x10,
    LF_DONTLIGHTACTORS = 0x20,
    LF_SPOT          = 0x40,
    LF_DONTLIGHTOTHERS = 0x80,
    LF_DONTLIGHTMAP  = 0x100
};

// Dynamic light structure
struct DynamicLight {
    float x, y, z;          // Position
    float color[3];          // RGB color
    float radius;            // Light radius
    float intensity;         // Brightness
    LightType type;          // Light type
    uint32_t flags;         // Light flags

    DynamicLight() : x(0), y(0), z(0), radius(256.0f), intensity(1.0f),
                    type(LIGHT_POINT), flags(LF_ADDITIVE | LF_ATTENUATE) {
        color[0] = color[1] = color[2] = 1.0f;
    }
};

// Light list for a surface
struct SurfaceLightList {
    int numLights;
    DynamicLight** lights;
    float* distances;

    SurfaceLightList() : numLights(0), lights(nullptr), distances(nullptr) {}
};

// Deferred renderer class
// Phase 2.2: G-buffer MRT FBO (normals) for higher-quality SSAO.
// Phase 3.1: full screen-space deferred dynamic lighting accumulation pass.
class DeferredRenderer {
private:
    bool useDeferred;
    int maxLightsPerSurface;

    // Feature flags
    bool bloomEnabled;
    float bloomIntensity;
    float fogDensity;
    float fogColor[3];

    // --- G-buffer resources (refs from PostFX — NOT owned by DeferredRenderer) ---
    // scene_fbo is the MRT FBO: attachment 0 = scene color, attachment 1 = normals,
    // attachment 2 = albedo (Phase 3.1b).
    // BindGBuffer() sets draw buffers [0,1,2]; BeginLightingPass() resets to [0].
    GLuint stored_scene_fbo;         ///< scene_fbo from PostFX
    GLuint stored_gbuf_norm_tex;     ///< Normal texture (attachment 1 of scene_fbo)
    GLuint stored_gbuf_albedo_tex;   ///< Pre-fog albedo texture (attachment 2 of scene_fbo; Phase 3.1b)
    GLuint stored_scene_depth_tex;   ///< Depth texture (owned by PostFX)
    int    gbuf_width;
    int    gbuf_height;
    bool   gbuf_ready;      ///< True when InitGBuffer() succeeded

    // --- Light accumulation FBO (Phase 3.1, owned by DeferredRenderer) ---
    // Completely independent from scene_fbo — no shared texture references.
    // This guarantees a clean FBO transition (no driver render-cache hazard).
    GLuint light_accum_tex;   ///< RGBA16F, same size as scene; receives additive light sum
    GLuint light_accum_fbo;   ///< Colour-only FBO with light_accum_tex; NO depth attachment

    GLuint deferred_prog;   ///< Linked VS+FS for the per-light fullscreen pass
    GLuint deferred_vao;    ///< Empty VAO for the fullscreen triangle draw
    GLuint null_cube_tex;   ///< 1×1 white R32F cube map — bound to unit 3 when no shadow
    struct {
        GLint uNormal, uDepth, uAlbedo;   ///< sampler2D units 0/1/2
        GLint uProjA, uProjB;             ///< cached_proj[10], cached_proj[14]
        GLint uProjSX, uProjSY;           ///< cached_proj[0],  cached_proj[5]
        GLint uLightEye;                  ///< vec4 eye-space xyz + radius
        GLint uLightColor;                ///< vec3 RGB
        GLint uShadowCube;                ///< samplerCube unit 3 (Phase 3.2)
        GLint uShadowCubeActive;          ///< int: 1 if cube shadow is bound
        GLint uInvView;                   ///< mat3: inverse view rotation (eye→world dir)
        GLint uSpecStrength;              ///< float: Blinn-Phong specular strength [0,1] (Phase 5.1)
        GLint uSpecExp;                   ///< float: Blinn-Phong specular exponent [4,128] (Phase 5.1)
    } dloc;
    bool deferred_ready;    ///< True when InitDeferredLightPass() succeeded

    bool InitDeferredLightPass();
    void DestroyDeferredLightPass();

    static GLuint CompileShader(GLenum type, const char *src);

public:
    DeferredRenderer();
    ~DeferredRenderer();

    // Initialize deferred renderer (feature flag setup)
    bool Init(int width, int height);

    // Initialize G-buffer: store refs to PostFX FBO resources and create the
    // independent light_accum_fbo (no shared textures).
    // scene_fbo must already have gbuf_norm_tex as GL_COLOR_ATTACHMENT1 and
    // gbuf_albedo_tex as GL_COLOR_ATTACHMENT2 (Phase 3.1b).
    // Returns true on success.
    bool InitGBuffer(unsigned int scene_fbo, unsigned int gbuf_norm_tex,
                     unsigned int gbuf_albedo_tex, unsigned int scene_depth_tex,
                     int w, int h);

    // Destroy light_accum_fbo/tex and clear stored refs. Does NOT delete shared textures.
    void DestroyGBuffer();

    // Enable MRT draw buffers [0,1] on the already-bound scene_fbo and clear
    // light_accum_tex so stale data from the previous frame is removed.
    void BindGBuffer();

    // True when the G-buffer FBO is ready for use.
    bool IsGBufferReady() const { return gbuf_ready; }

    // True when the G-buffer and the deferred light shader are both ready.
    // Gates: (a) the 0.1f ambient floor for dark sectors, and
    // (b) suppression of forward dynamic lights (replaced by the screen-space pass).
    bool IsDeferredLightReady() const { return gbuf_ready && deferred_ready; }

    // True when light_accum_fbo is allocated and the deferred light accumulation
    // pass can safely run (no shared textures — no feedback-loop hazard).
    bool IsDeferredPassSafe() const { return gbuf_ready && deferred_ready && light_accum_fbo != 0; }

    // Retrieve the G-buffer normal texture for SSAO sampling.
    unsigned int GetNormalTex() const { return stored_gbuf_norm_tex; }

    // Retrieve the pre-fog albedo texture (Phase 3.1b).
    unsigned int GetAlbedoTex() const { return stored_gbuf_albedo_tex; }

    // Retrieve the light accumulation texture for PostFX compositing.
    unsigned int GetLightAccumTex() const { return light_accum_tex; }

    // Set rendering mode
    void SetDeferredMode(bool deferred) { useDeferred = deferred; }
    bool IsDeferredMode() const { return useDeferred; }

    // Phase 3.1 / 3.2 lighting pass:
    // Call BeginLightingPass (caller must have bound the scene FBO first), then
    // one RenderLight per dynamic light, then EndLightingPass.
    // depth_tex / normal_tex: textures from PostFX / G-buffer.
    // projA = cached_proj[10], projB = cached_proj[14],
    // projSX = cached_proj[0], projSY = cached_proj[5].
    // invView: column-major mat3 = transpose of view matrix rotation (eye→world).
    //   Pass NULL if cube shadow maps are not in use (shadow sampling skipped).
    void BeginLightingPass(GLuint depth_tex, GLuint normal_tex,
                           GLfloat projA, GLfloat projB, GLfloat projSX, GLfloat projSY,
                           const GLfloat *invView = NULL);

    // ex/ey/ez: eye-space light position; radius: attenuation radius; r/g/b: color [0,1].
    // shadow_cube_tex: GL_TEXTURE_CUBE_MAP id for omnidirectional shadow (Phase 3.2),
    //   or 0 for unshadowed light.
    void RenderLight(float ex, float ey, float ez, float radius, float r, float g, float b,
                     GLuint shadow_cube_tex = 0);

    void EndLightingPass();

    // Geometry pass stubs (reserved for future full-deferred path)
    void BeginGeometryPass();
    void EndGeometryPass();

    // Post-processing stubs
    void ApplyBloom();
    void ApplyFog(float viewZ);
    void BeginPostProcess();
    void EndPostProcess();

    // Toggle features
    void EnableBloom(bool enable) { bloomEnabled = enable; }
    void SetBloomIntensity(float intensity) { bloomIntensity = intensity; }
    void SetFogDensity(float density) { fogDensity = density; }
    void SetFogColor(float r, float g, float b) { fogColor[0] = r; fogColor[1] = g; fogColor[2] = b; }

    // Cleanup (includes G-buffer and deferred light resources)
    void Destroy();

    // Print a one-line diagnostic state dump to the console.
    void LogStatus() const;

    // Check if deferred rendering is available
    bool IsAvailable() const { return gbuf_ready; }
};

// Global instance
extern DeferredRenderer* g_deferredRenderer;

#endif // hw_gbuffer_h
