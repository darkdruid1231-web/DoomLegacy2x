// hw_quadbatch.h — Streaming VBO quad batcher (Phase 1.2)
#ifndef hw_quadbatch_h
#define hw_quadbatch_h 1

#if defined(_WIN32) || defined(__MINGW32__)
#define GLEW_STATIC
#include <GL/glew.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#endif

/// Vertex format for the quad batcher.
/// Attribute layout must match GL 3.3 compat-profile aliases:
///   loc 0 = position (gl_Vertex), loc 3 = color (gl_Color), loc 8 = texcoord0 (gl_MultiTexCoord0)
struct QuadVertex {
    float x, y, z;       // position  (location 0)
    float s, t;          // texcoord  (location 8 — gl_MultiTexCoord0 alias)
    float r, g, b, a;    // color     (location 3 — gl_Color alias)
};

#define QUADBATCH_MAX 4096  ///< Maximum number of quads in a single batch

/// Streaming quad batcher using a single dynamic-draw VBO.
/// Quads are added to a CPU staging area, then flushed to GPU in one
/// glBufferSubData + glDrawArrays(GL_QUADS) call.
class QuadBatch
{
public:
    QuadBatch();

    /// Allocate GL resources. Must be called after a GL context is available.
    void Init();

    /// Release GL resources.
    void Destroy();

    /// Add a quad to the batch. corners[4] should be in winding order
    /// (e.g. bottom-left, bottom-right, top-right, top-left).
    /// Returns false and auto-flushes if the batch is full.
    bool AddQuad(const QuadVertex corners[4]);

    /// Upload staged quads to GPU and draw them. Resets the batch.
    void Flush();

    /// Discard all staged quads without drawing.
    void Reset();

    /// Number of quads currently staged.
    int Count() const { return quad_count; }

private:
    GLuint vao_id;
    GLuint vbo_id;
    QuadVertex staging[QUADBATCH_MAX * 4];
    int quad_count;
};

/// Global quad batcher for world geometry (wall segs, flats, 2D).
extern QuadBatch quadbatch;

#endif // hw_quadbatch_h
