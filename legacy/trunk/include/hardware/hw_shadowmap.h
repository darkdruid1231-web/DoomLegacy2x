// hw_shadowmap.h — Single-light depth-only shadow map (Phase 2.3)
#ifndef hw_shadowmap_h
#define hw_shadowmap_h 1

#if defined(_WIN32) || defined(__MINGW32__)
#define GLEW_STATIC
#include <GL/glew.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#endif

/// Depth-only shadow map rendered from the nearest dynamic light's perspective.
/// Provides a single 2D shadow map; omnidirectional cube maps are Phase 2.3b.
class ShadowMap {
public:
    static const int DEFAULT_RES = 1024;

    ShadowMap();
    ~ShadowMap();

    /// Allocate depth FBO, texture, and compile depth-only shader.
    /// Call after GL context is ready.  Safe to call multiple times (re-inits).
    bool Init(int resolution = DEFAULT_RES);

    /// Release all GL resources.
    void Destroy();

    /// Begin shadow depth pass.
    /// lx/ly/lz: light world position.
    /// tx/ty/tz: target (player camera) world position — the light "looks" here.
    /// light_radius: range of the light (sets the far clip plane).
    void BeginPass(float lx, float ly, float lz,
                   float tx, float ty, float tz,
                   float light_radius);

    /// End shadow pass.  Restores restore_fbo and the given viewport.
    void EndPass(GLuint restore_fbo, int vpW, int vpH);

    bool         IsReady()      const { return fbo_ready; }
    GLuint       GetDepthTex()  const { return depth_tex; }
    GLuint       GetDepthProg() const { return depth_prog; }
    GLint        GetMVPLoc()    const { return loc_lightmvp; }

    /// Shadow matrix: world-space → shadow texture coords [0,1]³  (bias * proj * view).
    /// Upload to uShadowMatrix in the surface shader.
    const GLfloat* GetLightVP() const { return light_vp; }

private:
    bool   fbo_ready;
    int    res;
    GLuint fbo;
    GLuint depth_tex;
    GLuint depth_prog;    ///< GLSL depth-only program
    GLint  loc_lightmvp;  ///< uLightMVP uniform location in depth_prog
    GLfloat light_vp[16]; ///< bias * proj * view (world → shadow texture coords)

    static void Mat4LookAt(GLfloat *m,
                           float ex, float ey, float ez,
                           float cx, float cy, float cz);
    static void Mat4Perspective(GLfloat *m, float fovY_deg, float aspect,
                                float nearZ, float farZ);
    static void Mat4Mul(GLfloat *result, const GLfloat *A, const GLfloat *B);
};

extern ShadowMap shadowmap;

// ---------------------------------------------------------------------------
// Omnidirectional cube shadow map (Phase 3.2 / Phase 2.3b)
//
// Uses a GL_R32F cube-map texture storing normalized linear distance
// (dist / lightRadius) so that the deferred shader can do a simple
// scalar comparison without a sampler2DShadow PCF setup.
// Six face-FBOs share one depth renderbuffer; each face is rendered
// with a 90° view looking along one of the six principal world axes.
// ---------------------------------------------------------------------------
class CubeShadowMap {
public:
    static const int DEFAULT_RES   = 256;  ///< Per-face resolution (256×256)
    static const int MAX_INSTANCES = 4;    ///< Size of cube_shadow_pool[]

    CubeShadowMap();
    ~CubeShadowMap();

    bool Init(int resolution = DEFAULT_RES);
    void Destroy();

    /// Set light world-position + radius and configure depth state.
    /// Call once per light before iterating BeginFacePass()/EndFacePass().
    void BeginCube(float lx, float ly, float lz, float light_radius);

    /// Bind face FBO f (0-5) and upload per-face MVP.
    /// Caller renders geometry (e.g. RenderShadowBSPNode) then calls EndFacePass.
    void BeginFacePass(int face);

    /// No-op — next BeginFacePass() will rebind the new face FBO.
    void EndFacePass() {}

    /// Restore the main FBO + viewport after all 6 faces have been rendered.
    void EndCube(GLuint restore_fbo, int vpW, int vpH);

    GLuint GetCubeTex()  const { return cube_tex; }
    bool   IsReady()     const { return fbo_ready; }

private:
    bool   fbo_ready;
    int    res;
    GLuint face_fbo[6];    ///< One FBO per cube face (colour = cube_tex face)
    GLuint cube_tex;       ///< GL_TEXTURE_CUBE_MAP, GL_R32F (linear depth)
    GLuint depth_rbo;      ///< Shared depth renderbuffer for all face passes
    GLuint depth_prog;     ///< GLSL program: writes linear dist to colour output
    GLint  loc_lightmvp;   ///< uLightMVP uniform
    GLint  loc_lightpos;   ///< uLightPos uniform (world-space)
    GLint  loc_lightradius;///< uLightRadius uniform

    float  cur_lx, cur_ly, cur_lz, cur_radius; ///< Set by BeginCube()

    struct FaceDir { float dx, dy, dz, ux, uy, uz; };
    static const FaceDir s_face_dirs[6]; ///< Look + up directions per face

    static void Mat4LookAt(GLfloat *m,
                           float ex, float ey, float ez,
                           float cx, float cy, float cz,
                           float ux, float uy, float uz);
    static void Mat4Perspective(GLfloat *m, float fovY_deg, float aspect,
                                float nearZ, float farZ);
    static void Mat4Mul(GLfloat *out, const GLfloat *A, const GLfloat *B);
    static GLuint CompileShader(GLenum type, const char *src);
};

/// Pool of cube shadow maps; indexed [0..cv_grcubeshadows-1].
extern CubeShadowMap cube_shadow_pool[CubeShadowMap::MAX_INSTANCES];

#endif // hw_shadowmap_h
