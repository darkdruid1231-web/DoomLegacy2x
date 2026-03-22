// hw_quadbatch.cpp — Streaming VBO quad batcher (Phase 1.2)

#define GL_GLEXT_PROTOTYPES 1
#if defined(_WIN32) || defined(__MINGW32__)
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <cstring>
#include <cstddef>

#include "hw_quadbatch.h"

QuadBatch quadbatch;

QuadBatch::QuadBatch() : vao_id(0), vbo_id(0), quad_count(0) {}

void QuadBatch::Init()
{
    if (vao_id) return;  // already initialized

    glGenVertexArrays(1, &vao_id);
    glGenBuffers(1, &vbo_id);

    glBindVertexArray(vao_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    // Pre-allocate max capacity; we will use glBufferSubData to update
    glBufferData(GL_ARRAY_BUFFER, sizeof(staging), NULL, GL_DYNAMIC_DRAW);

    // Position at location 0 (aliases gl_Vertex in compat profile)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          (void*)offsetof(QuadVertex, x));
    glEnableVertexAttribArray(0);

    // TexCoord at location 8 (aliases gl_MultiTexCoord0 in compat profile)
    glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          (void*)offsetof(QuadVertex, s));
    glEnableVertexAttribArray(8);

    // Color at location 3 (aliases gl_Color in compat profile)
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          (void*)offsetof(QuadVertex, r));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    quad_count = 0;
}

void QuadBatch::Destroy()
{
    if (vao_id) { glDeleteVertexArrays(1, &vao_id); vao_id = 0; }
    if (vbo_id) { glDeleteBuffers(1, &vbo_id);       vbo_id = 0; }
    quad_count = 0;
}

bool QuadBatch::AddQuad(const QuadVertex corners[4])
{
    if (quad_count >= QUADBATCH_MAX)
        Flush();
    QuadVertex *dst = &staging[quad_count * 4];
    dst[0] = corners[0];
    dst[1] = corners[1];
    dst[2] = corners[2];
    dst[3] = corners[3];
    quad_count++;
    return true;
}

void QuadBatch::Flush()
{
    if (!quad_count || !vao_id) return;
    int vert_count = quad_count * 4;
    glBindVertexArray(vao_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vert_count * sizeof(QuadVertex), staging);
    glDrawArrays(GL_QUADS, 0, vert_count);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    quad_count = 0;
}

void QuadBatch::Reset()
{
    quad_count = 0;
}
