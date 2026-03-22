// hw_shadowmap.cpp — Shadow map implementation (Phase 2.3)
#define GL_GLEXT_PROTOTYPES 1
#if defined(_WIN32) || defined(__MINGW32__)
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <cmath>
#include <cstring>
#include "hardware/hw_shadowmap.h"
#include "doomdef.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ShadowMap shadowmap;

// ---------------------------------------------------------------------------
// Depth-only shaders
// ---------------------------------------------------------------------------
static const char *depth_vert_src =
    "#version 330 compatibility\n"
    "layout(location=0) in vec4 aPosition;\n"
    "uniform mat4 uLightMVP;\n"
    "void main() {\n"
    "    gl_Position = uLightMVP * aPosition;\n"
    "}\n";

static const char *depth_frag_src =
    "#version 330 compatibility\n"
    "void main() {}\n";   // depth writes happen automatically

// ---------------------------------------------------------------------------
ShadowMap::ShadowMap()
    : fbo_ready(false), res(DEFAULT_RES), fbo(0), depth_tex(0),
      depth_prog(0), loc_lightmvp(-1)
{
    memset(light_vp, 0, sizeof(light_vp));
}

ShadowMap::~ShadowMap() { Destroy(); }

bool ShadowMap::Init(int resolution)
{
    Destroy();
    res = resolution;

    // Depth texture with PCF comparison mode
    glGenTextures(1, &depth_tex);
    glBindTexture(GL_TEXTURE_2D, depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, res, res, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // Border = 1.0: outside shadow frustum → fully lit (no false shadow)
    static const GLfloat border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    // PCF hardware comparison: sampler2DShadow uses GL_COMPARE_REF_TO_TEXTURE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Depth-only FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, depth_tex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        CONS_Printf("ShadowMap: depth FBO incomplete (0x%x).\n", status);
        Destroy();
        return false;
    }

    // Compile depth-only GLSL program
    auto compile = [](GLenum type, const char *src) -> GLuint {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, NULL);
        glCompileShader(sh);
        GLint ok = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) { glDeleteShader(sh); return 0; }
        return sh;
    };

    GLuint vs = compile(GL_VERTEX_SHADER,   depth_vert_src);
    GLuint fs = compile(GL_FRAGMENT_SHADER, depth_frag_src);
    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        CONS_Printf("ShadowMap: depth shader compilation failed.\n");
        Destroy(); return false;
    }

    depth_prog = glCreateProgram();
    glAttachShader(depth_prog, vs);
    glAttachShader(depth_prog, fs);
    glLinkProgram(depth_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(depth_prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        CONS_Printf("ShadowMap: depth program link failed.\n");
        Destroy(); return false;
    }

    loc_lightmvp = glGetUniformLocation(depth_prog, "uLightMVP");
    fbo_ready = true;
    CONS_Printf("ShadowMap: %dx%d shadow map ready.\n", res, res);
    return true;
}

void ShadowMap::Destroy()
{
    if (depth_prog) { glDeleteProgram(depth_prog); depth_prog = 0; }
    if (fbo)        { glDeleteFramebuffers(1, &fbo); fbo = 0; }
    if (depth_tex)  { glDeleteTextures(1, &depth_tex); depth_tex = 0; }
    fbo_ready = false;
}

// ---------------------------------------------------------------------------
// Matrix helpers (column-major, matching OpenGL convention)
// ---------------------------------------------------------------------------

void ShadowMap::Mat4LookAt(GLfloat *m,
                           float ex, float ey, float ez,
                           float cx, float cy, float cz)
{
    // forward = normalize(eye - center)  (we look down -Z)
    float fx = ex-cx, fy = ey-cy, fz = ez-cz;
    float fl = sqrtf(fx*fx + fy*fy + fz*fz);
    if (fl < 1e-6f) { fz = 1.0f; fl = 1.0f; }
    fx /= fl; fy /= fl; fz /= fl;

    // Choose an up vector that is not parallel to forward
    float upx = 0.0f, upy = 0.0f, upz = 1.0f;
    if (fabsf(fz) > 0.99f) { upx = 1.0f; upz = 0.0f; }

    // right = normalize(up × forward)
    float rx = upy*fz - upz*fy,  ry = upz*fx - upx*fz,  rz = upx*fy - upy*fx;
    float rl = sqrtf(rx*rx + ry*ry + rz*rz);
    if (rl < 1e-6f) rl = 1.0f;
    rx /= rl; ry /= rl; rz /= rl;

    // true up = forward × right
    float vx = fy*rz - fz*ry,  vy = fz*rx - fx*rz,  vz = fx*ry - fy*rx;

    // Column-major: col0=(rx,vx,fx,0), col1=(ry,vy,fy,0), col2=(rz,vz,fz,0), col3=(tx,ty,tz,1)
    m[0]=rx; m[4]=ry; m[8] =rz; m[12]=-(rx*ex + ry*ey + rz*ez);
    m[1]=vx; m[5]=vy; m[9] =vz; m[13]=-(vx*ex + vy*ey + vz*ez);
    m[2]=fx; m[6]=fy; m[10]=fz; m[14]=-(fx*ex + fy*ey + fz*ez);
    m[3]=0;  m[7]=0;  m[11]=0;  m[15]=1.0f;
}

void ShadowMap::Mat4Perspective(GLfloat *m, float fovY_deg, float aspect,
                                float nearZ, float farZ)
{
    float f = 1.0f / tanf(fovY_deg * (float)(M_PI / 360.0));
    memset(m, 0, 16 * sizeof(GLfloat));
    m[0]  =  f / aspect;
    m[5]  =  f;
    m[10] =  (farZ + nearZ) / (nearZ - farZ);
    m[11] = -1.0f;
    m[14] =  (2.0f * farZ * nearZ) / (nearZ - farZ);
}

void ShadowMap::Mat4Mul(GLfloat *result, const GLfloat *A, const GLfloat *B)
{
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++)
                sum += A[k*4 + row] * B[col*4 + k];
            result[col*4 + row] = sum;
        }
}

// ---------------------------------------------------------------------------
// Pass management
// ---------------------------------------------------------------------------

void ShadowMap::BeginPass(float lx, float ly, float lz,
                          float tx, float ty, float tz,
                          float light_radius)
{
    if (!fbo_ready) return;

    // Build light-space view-projection matrix
    GLfloat view[16], proj[16], vp[16];
    Mat4LookAt(view, lx, ly, lz, tx, ty, tz);

    float nearZ = 8.0f;
    float farZ  = (light_radius > nearZ * 2.0f) ? light_radius : nearZ * 2.0f;
    Mat4Perspective(proj, 90.0f, 1.0f, nearZ, farZ);
    Mat4Mul(vp, proj, view);

    // Bias matrix: NDC [-1,1]³ → texture coords [0,1]³ (column-major)
    static const GLfloat bias[16] = {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f
    };
    Mat4Mul(light_vp, bias, vp);  // shadow matrix = bias * proj * view

    // Bind shadow FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, res, res);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Depth-only state
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);   // Render back faces → reduces peter-panning artefacts
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    // Activate depth shader and upload raw vp (bias NOT included — shader does raw projection)
    glUseProgram(depth_prog);
    if (loc_lightmvp >= 0)
        glUniformMatrix4fv(loc_lightmvp, 1, GL_FALSE, vp);
}

void ShadowMap::EndPass(GLuint restore_fbo, int vpW, int vpH)
{
    if (!fbo_ready) return;

    glUseProgram(0);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.0f, 0.0f);
    glCullFace(GL_BACK);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, restore_fbo);
    glViewport(0, 0, vpW, vpH);
}

// ===========================================================================
// CubeShadowMap — Phase 3.2 omnidirectional point-light shadow maps
// ===========================================================================

CubeShadowMap cube_shadow_pool[CubeShadowMap::MAX_INSTANCES];

// ---------------------------------------------------------------------------
// Cube depth-pass shaders
// ---------------------------------------------------------------------------

// Vertex: transforms by light MVP and passes world-space position through.
// layout(location=0) is attribute 0 = gl_Vertex in compat mode (glVertex3f).
static const char *cube_depth_vert_src =
    "#version 330 compatibility\n"
    "layout(location=0) in vec4 aPosition;\n"
    "uniform mat4 uLightMVP;\n"
    "out vec3 v_worldpos;\n"
    "void main() {\n"
    "    v_worldpos  = aPosition.xyz;\n"
    "    gl_Position = uLightMVP * aPosition;\n"
    "}\n";

// Fragment: writes normalized linear distance to the R32F colour attachment.
// Clearing the attachment to 1.0 before each face means "fully lit at max range".
static const char *cube_depth_frag_src =
    "#version 330 compatibility\n"
    "in vec3 v_worldpos;\n"
    "uniform vec3  uLightPos;\n"
    "uniform float uLightRadius;\n"
    "layout(location=0) out float fragLinearDepth;\n"
    "void main() {\n"
    "    float dist = length(v_worldpos - uLightPos) / uLightRadius;\n"
    "    fragLinearDepth = clamp(dist, 0.0, 1.0);\n"
    "}\n";

// ---------------------------------------------------------------------------
// Face look/up directions — must match OpenGL cube-map sampling convention
// (face order: +X, -X, +Y, -Y, +Z, -Z  = GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0..5)
// ---------------------------------------------------------------------------
const CubeShadowMap::FaceDir CubeShadowMap::s_face_dirs[6] = {
    { 1, 0, 0,   0,-1, 0},   // +X: look +X, up -Y
    {-1, 0, 0,   0,-1, 0},   // -X: look -X, up -Y
    { 0, 1, 0,   0, 0, 1},   // +Y: look +Y, up +Z
    { 0,-1, 0,   0, 0,-1},   // -Y: look -Y, up -Z
    { 0, 0, 1,   0,-1, 0},   // +Z: look +Z, up -Y
    { 0, 0,-1,   0,-1, 0},   // -Z: look -Z, up -Y
};

// ---------------------------------------------------------------------------
// CubeShadowMap helpers
// ---------------------------------------------------------------------------

GLuint CubeShadowMap::CompileShader(GLenum type, const char *src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) { glDeleteShader(sh); return 0; }
    return sh;
}

void CubeShadowMap::Mat4LookAt(GLfloat *m,
                                float ex, float ey, float ez,
                                float cx, float cy, float cz,
                                float ux, float uy, float uz)
{
    // forward = normalize(eye - center)  (camera looks down -Z)
    float fx = ex-cx, fy = ey-cy, fz = ez-cz;
    float fl = sqrtf(fx*fx + fy*fy + fz*fz);
    if (fl < 1e-6f) { fz = 1.0f; fl = 1.0f; }
    fx /= fl; fy /= fl; fz /= fl;

    // right = normalize(up × forward)
    float rx = uy*fz - uz*fy, ry = uz*fx - ux*fz, rz = ux*fy - uy*fx;
    float rl = sqrtf(rx*rx + ry*ry + rz*rz);
    if (rl < 1e-6f) rl = 1.0f;
    rx /= rl; ry /= rl; rz /= rl;

    // true up = forward × right
    float vx = fy*rz - fz*ry, vy = fz*rx - fx*rz, vz = fx*ry - fy*rx;

    // Column-major view matrix
    m[0]=rx; m[4]=ry; m[8] =rz; m[12]=-(rx*ex + ry*ey + rz*ez);
    m[1]=vx; m[5]=vy; m[9] =vz; m[13]=-(vx*ex + vy*ey + vz*ez);
    m[2]=fx; m[6]=fy; m[10]=fz; m[14]=-(fx*ex + fy*ey + fz*ez);
    m[3]=0;  m[7]=0;  m[11]=0;  m[15]=1.0f;
}

void CubeShadowMap::Mat4Perspective(GLfloat *m, float fovY_deg, float aspect,
                                     float nearZ, float farZ)
{
    float f = 1.0f / tanf(fovY_deg * (float)(M_PI / 360.0));
    memset(m, 0, 16 * sizeof(GLfloat));
    m[0]  =  f / aspect;
    m[5]  =  f;
    m[10] =  (farZ + nearZ) / (nearZ - farZ);
    m[11] = -1.0f;
    m[14] =  (2.0f * farZ * nearZ) / (nearZ - farZ);
}

void CubeShadowMap::Mat4Mul(GLfloat *out, const GLfloat *A, const GLfloat *B)
{
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++)
                sum += A[k*4 + row] * B[col*4 + k];
            out[col*4 + row] = sum;
        }
}

// ---------------------------------------------------------------------------
// CubeShadowMap constructor / destructor
// ---------------------------------------------------------------------------

CubeShadowMap::CubeShadowMap()
    : fbo_ready(false), res(DEFAULT_RES), cube_tex(0), depth_rbo(0),
      depth_prog(0), loc_lightmvp(-1), loc_lightpos(-1), loc_lightradius(-1),
      cur_lx(0), cur_ly(0), cur_lz(0), cur_radius(256.0f)
{
    memset(face_fbo, 0, sizeof(face_fbo));
}

CubeShadowMap::~CubeShadowMap() { Destroy(); }

// ---------------------------------------------------------------------------
// Init / Destroy
// ---------------------------------------------------------------------------

bool CubeShadowMap::Init(int resolution)
{
    Destroy();
    res = resolution;

    // --- Cube colour texture: GL_R32F, one face per image target ---
    glGenTextures(1, &cube_tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    for (int f = 0; f < 6; f++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f,
                     0, GL_R32F, res, res, 0, GL_RED, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // --- Shared depth renderbuffer for all 6 faces ---
    glGenRenderbuffers(1, &depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, res, res);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // --- One FBO per face: colour = that cube face, depth = shared RBO ---
    for (int f = 0; f < 6; f++)
    {
        glGenFramebuffers(1, &face_fbo[f]);
        glBindFramebuffer(GL_FRAMEBUFFER, face_fbo[f]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, cube_tex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depth_rbo);
        GLenum draw = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &draw);

        GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (st != GL_FRAMEBUFFER_COMPLETE)
        {
            CONS_Printf("CubeShadowMap: face %d FBO incomplete (0x%x).\n", f, st);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            Destroy();
            return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // --- Compile depth GLSL program ---
    GLuint vs = CompileShader(GL_VERTEX_SHADER,   cube_depth_vert_src);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, cube_depth_frag_src);
    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        CONS_Printf("CubeShadowMap: depth shader failed to compile.\n");
        Destroy();
        return false;
    }

    depth_prog = glCreateProgram();
    glAttachShader(depth_prog, vs);
    glAttachShader(depth_prog, fs);
    glLinkProgram(depth_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(depth_prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        CONS_Printf("CubeShadowMap: depth program link failed.\n");
        Destroy();
        return false;
    }

    loc_lightmvp    = glGetUniformLocation(depth_prog, "uLightMVP");
    loc_lightpos    = glGetUniformLocation(depth_prog, "uLightPos");
    loc_lightradius = glGetUniformLocation(depth_prog, "uLightRadius");

    fbo_ready = true;
    CONS_Printf("CubeShadowMap: %dx%d cube shadow map ready.\n", res, res);
    return true;
}

void CubeShadowMap::Destroy()
{
    if (depth_prog) { glDeleteProgram(depth_prog); depth_prog = 0; }
    for (int f = 0; f < 6; f++)
        if (face_fbo[f]) { glDeleteFramebuffers(1, &face_fbo[f]); face_fbo[f] = 0; }
    if (depth_rbo) { glDeleteRenderbuffers(1, &depth_rbo); depth_rbo = 0; }
    if (cube_tex)  { glDeleteTextures(1, &cube_tex);  cube_tex  = 0; }
    loc_lightmvp = loc_lightpos = loc_lightradius = -1;
    fbo_ready = false;
}

// ---------------------------------------------------------------------------
// Pass management
// ---------------------------------------------------------------------------

void CubeShadowMap::BeginCube(float lx, float ly, float lz, float light_radius)
{
    cur_lx = lx; cur_ly = ly; cur_lz = lz;
    cur_radius = (light_radius > 1.0f) ? light_radius : 1.0f;

    // Set depth/rasterizer state shared by all 6 face passes.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);          // front-face cull reduces self-shadowing
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void CubeShadowMap::BeginFacePass(int face)
{
    if (!fbo_ready || face < 0 || face >= 6) return;

    const FaceDir &d = s_face_dirs[face];

    // Build 90° perspective + look-at view for this face.
    float tx = cur_lx + d.dx, ty = cur_ly + d.dy, tz = cur_lz + d.dz;
    GLfloat view[16], proj[16], vp[16];
    Mat4LookAt(view, cur_lx, cur_ly, cur_lz, tx, ty, tz, d.ux, d.uy, d.uz);

    float nearZ = 4.0f;
    float farZ  = (cur_radius > nearZ * 2.0f) ? cur_radius : nearZ * 2.0f;
    Mat4Perspective(proj, 90.0f, 1.0f, nearZ, farZ);
    Mat4Mul(vp, proj, view);

    glBindFramebuffer(GL_FRAMEBUFFER, face_fbo[face]);
    glViewport(0, 0, res, res);
    // Clear linear depth to 1.0 (= full radius = fully lit) and reset depth buffer.
    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(depth_prog);
    if (loc_lightmvp    >= 0) glUniformMatrix4fv(loc_lightmvp, 1, GL_FALSE, vp);
    if (loc_lightpos    >= 0) glUniform3f(loc_lightpos, cur_lx, cur_ly, cur_lz);
    if (loc_lightradius >= 0) glUniform1f(loc_lightradius, cur_radius);
}

void CubeShadowMap::EndCube(GLuint restore_fbo, int vpW, int vpH)
{
    if (!fbo_ready) return;
    glUseProgram(0);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.0f, 0.0f);
    glCullFace(GL_BACK);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, restore_fbo);
    glViewport(0, 0, vpW, vpH);
}
