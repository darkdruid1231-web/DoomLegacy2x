//-----------------------------------------------------------------------------
//
// $Id: gl_bsp.cpp $
//
// Dynamic GL node generation for Doom Legacy
//
// Implements proper GL node generation following zdbsp approach:
// - Minisegs to fill gaps between wall vertices
// - Clockwise ordering by sorting ALL vertices from centroid
// - Degenerate subsector detection
//-----------------------------------------------------------------------------

#include <string.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include "doomdata.h"
#include "g_map.h"
#include "doomdef.h"
#include "p_setup.h"
#include "r_data.h"
#include "z_zone.h"
#include "r_defs.h"

// Data structure for GL seg (like zdbsp's approach)
struct GLSeg {
    int v1Index;
    int v2Index;
    bool isMiniseg;
    int linedef;
    int side;

    GLSeg() : v1Index(0), v2Index(0), isMiniseg(true), linedef(-1), side(0) {}
    GLSeg(int v1, int v2, bool mini, int line = -1, int s = 0)
        : v1Index(v1), v2Index(v2), isMiniseg(mini), linedef(line), side(s) {}
};

// Data structure for unique vertices with angle
struct GLVertex {
    float x, y;
    int originalIndex;
    float angle;

    GLVertex() : x(0), y(0), originalIndex(-1), angle(0) {}
    GLVertex(float x_, float y_, int idx = -1) : x(x_), y(y_), originalIndex(idx), angle(0) {}
};

// Calculate angle from point to centroid
static float AngleToCentroid(float x, float y, float cx, float cy)
{
    return atan2(y - cy, x - cx);
}

// Check if two vertices are connected by an existing seg
static bool VerticesConnected(const std::vector<GLSeg>& segs, int v1, int v2)
{
    for (const auto& seg : segs) {
        if ((seg.v1Index == v1 && seg.v2Index == v2) ||
            (seg.v1Index == v2 && seg.v2Index == v1)) {
            return true;
        }
    }
    return false;
}

// Function to find or add a GL vertex
static int AddGLVertex(Map* map, float x, float y)
{
    // Check if this vertex already exists
    for (int i = 0; i < map->numglvertexes; i++) {
        if (map->glvertexes[i].x.Float() == x && map->glvertexes[i].y.Float() == y) {
            return i;
        }
    }

    // Add new vertex
    int newIndex = map->numglvertexes;
    vertex_t* newVertexes = (vertex_t*)Z_Malloc((newIndex + 1) * sizeof(vertex_t), PU_LEVEL, 0);

    // Copy existing
    for (int i = 0; i < newIndex; i++) {
        newVertexes[i] = map->glvertexes[i];
    }

    if (map->glvertexes && map->numglvertexes > 0) {
        Z_Free(map->glvertexes);
    }

    map->glvertexes = newVertexes;
    map->glvertexes[newIndex].x = (fixed_t)(x * 65536.0f);
    map->glvertexes[newIndex].y = (fixed_t)(y * 65536.0f);
    map->numglvertexes++;

    return newIndex;
}

// Build proper GL nodes with minisegs
void BuildGLNodes(Map* map)
{
    CONS_Printf("Building GL nodes with minisegs...\n");

    // Step 1: Initialize glvertexes array
    map->glvertexes = (vertex_t*)Z_Malloc(map->numvertexes * sizeof(vertex_t), PU_LEVEL, 0);
    for (int i = 0; i < map->numvertexes; i++) {
        map->glvertexes[i].x = map->vertexes[i].x;
        map->glvertexes[i].y = map->vertexes[i].y;
    }
    map->numglvertexes = map->numvertexes;

    // Step 2: Process each subsector
    std::vector<GLSeg> allGLSegs;
    std::map<int, std::vector<GLSeg>> subsectorSegs;

    for (int s = 0; s < map->numsubsectors; s++) {
        subsector_t* sub = &map->subsectors[s];
        int firstSeg = sub->first_seg;
        int numSegs = sub->num_segs;

        if (numSegs < 3) {
            CONS_Printf("  Warning: Subsector %d has only %d segs\n", s, numSegs);
            continue;
        }

        // Collect unique vertices and build initial seg list
        std::vector<GLVertex> vertices;
        std::vector<GLSeg> wallSegs;
        std::set<int> vertexSet;

        for (int i = 0; i < numSegs; i++) {
            seg_t* seg = &map->segs[firstSeg + i];

            float v1x = seg->v1->x.Float();
            float v1y = seg->v1->y.Float();
            float v2x = seg->v2->x.Float();
            float v2y = seg->v2->y.Float();

            int v1Idx = AddGLVertex(map, v1x, v1y);
            int v2Idx = AddGLVertex(map, v2x, v2y);

            // Check if this is a miniseg (no linedef)
            bool isMiniseg = (seg->linedef == NULL);

            int lineIdx = -1;
            int sideIdx = 0;
            if (seg->linedef) {
                lineIdx = seg->linedef - map->lines;
                sideIdx = seg->side;
            }

            wallSegs.push_back(GLSeg(v1Idx, v2Idx, isMiniseg, lineIdx, sideIdx));

            vertexSet.insert(v1Idx);
            vertexSet.insert(v2Idx);
        }

        // Convert set to vector for processing
        vertices.reserve(vertexSet.size());
        for (int idx : vertexSet) {
            float vx = map->glvertexes[idx].x.Float();
            float vy = map->glvertexes[idx].y.Float();
            vertices.push_back(GLVertex(vx, vy, idx));
        }

        // Calculate centroid (midpoint)
        float accumx = 0, accumy = 0;
        for (const auto& v : vertices) {
            accumx += v.x;
            accumy += v.y;
        }
        float midx = accumx / vertices.size();
        float midy = accumy / vertices.size();

        // Calculate angle for each vertex from centroid
        for (auto& v : vertices) {
            v.angle = AngleToCentroid(v.x, v.y, midx, midy);
        }

        // Sort vertices by angle (counterclockwise from centroid)
        std::sort(vertices.begin(), vertices.end(),
            [](const GLVertex& a, const GLVertex& b) {
                return a.angle < b.angle;
            });

        // Add minisegs for gaps between consecutive vertices
        std::vector<GLSeg> orderedSegs = wallSegs;

        for (size_t i = 0; i < vertices.size(); i++) {
            size_t next = (i + 1) % vertices.size();
            int v1 = vertices[i].originalIndex;
            int v2 = vertices[next].originalIndex;

            // Check if these vertices are already connected
            if (!VerticesConnected(orderedSegs, v1, v2)) {
                // Add miniseg to fill the gap
                orderedSegs.push_back(GLSeg(v1, v2, true, -1, 0));
            }
        }

        // Degenerate subsector check - if all segs are minisegs, warn
        int minisegCount = 0;
        for (const auto& seg : orderedSegs) {
            if (seg.isMiniseg) minisegCount++;
        }
        if (minisegCount == (int)orderedSegs.size()) {
            CONS_Printf("  Warning: Subsector %d is all minisegs!\n", s);
        }

        // Store segs for this subsector
        subsectorSegs[s] = orderedSegs;

        // Add to global seg list
        for (const auto& seg : orderedSegs) {
            allGLSegs.push_back(seg);
        }
    }

    // Step 3: Create glsegs array
    map->glsegs = (seg_t*)Z_Malloc(allGLSegs.size() * sizeof(seg_t), PU_LEVEL, 0);
    map->numglsegs = allGLSegs.size();

    for (int i = 0; i < map->numglsegs; i++) {
        seg_t* glseg = &map->glsegs[i];
        const GLSeg& glSeg = allGLSegs[i];

        glseg->v1 = &map->glvertexes[glSeg.v1Index];
        glseg->v2 = &map->glvertexes[glSeg.v2Index];

        // Calculate angle and length
        if (glseg->v1 != glseg->v2) {
            float dx = glseg->v2->x.Float() - glseg->v1->x.Float();
            float dy = glseg->v2->y.Float() - glseg->v1->y.Float();
            glseg->angle = (angle_t)(atan2(dy, dx) * 65536.0 / (2.0 * M_PI));
            glseg->length = sqrt(dx * dx + dy * dy);
        } else {
            glseg->angle = 0;
            glseg->length = 0;
        }

        // Set linedef if available
        if (glSeg.linedef >= 0 && glSeg.linedef < map->numlines) {
            glseg->linedef = &map->lines[glSeg.linedef];
            glseg->side = glSeg.side;
        } else {
            glseg->linedef = NULL;
            glseg->side = 0;
        }

        glseg->sidedef = NULL;
        glseg->frontsector = NULL;
        glseg->backsector = NULL;
        glseg->numlights = 0;
        glseg->rlights = NULL;
    }

    // Step 4: Create glsubsectors
    map->glsubsectors = (subsector_t*)Z_Malloc(map->numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
    map->numglsubsectors = map->numsubsectors;

    int segOffset = 0;
    for (int s = 0; s < map->numsubsectors; s++) {
        subsector_t* sub = &map->subsectors[s];
        subsector_t* glsub = &map->glsubsectors[s];

        glsub->sector = sub->sector;
        glsub->first_seg = segOffset;
        glsub->num_segs = subsectorSegs[s].size();
        glsub->splats = sub->splats;
        glsub->poly = sub->poly;

        segOffset += glsub->num_segs;
    }

    // Step 5: Copy nodes
    map->glnodes = (node_t*)Z_Malloc(map->numnodes * sizeof(node_t), PU_LEVEL, 0);
    map->numglnodes = map->numnodes;
    for (int i = 0; i < map->numnodes; i++) {
        map->glnodes[i] = map->nodes[i];
    }

    CONS_Printf("GL nodes built: %d vertices, %d segs, %d subsectors\n",
        map->numglvertexes, map->numglsegs, map->numglsubsectors);
}

// Legacy compatibility functions
void BuildGLVertexes(Map *map)
{
    map->glvertexes = (vertex_t*)Z_Malloc(map->numvertexes * sizeof(vertex_t), PU_LEVEL, 0);
    for (int i = 0; i < map->numvertexes; i++) {
        map->glvertexes[i].x = map->vertexes[i].x;
        map->glvertexes[i].y = map->vertexes[i].y;
    }
    map->numglvertexes = map->numvertexes;
}

void BuildGLSegs(Map *map)
{
    if (!map->glsegs && map->glvertexes) {
        BuildGLVertexes(map);
    }

    if (map->glsegs) return;

    map->glsegs = (seg_t*)Z_Malloc(map->numsegs * sizeof(seg_t), PU_LEVEL, 0);
    for (int i = 0; i < map->numsegs; i++) {
        seg_t *seg = &map->segs[i];
        seg_t *glseg = &map->glsegs[i];

        if (seg->v1 && seg->v2 && map->glvertexes) {
            glseg->v1 = &map->glvertexes[seg->v1 - map->vertexes];
            glseg->v2 = &map->glvertexes[seg->v2 - map->vertexes];
        } else {
            glseg->v1 = seg->v1;
            glseg->v2 = seg->v2;
        }

        glseg->angle = seg->angle;
        glseg->offset = seg->offset;
        glseg->sidedef = seg->sidedef;
        glseg->linedef = seg->linedef;
        glseg->side = seg->side;
        glseg->frontsector = seg->frontsector;
        glseg->backsector = seg->backsector;

        float dx = (glseg->v2->x - glseg->v1->x).Float();
        float dy = (glseg->v2->y - glseg->v1->y).Float();
        glseg->length = sqrt(dx * dx + dy * dy);

        glseg->numlights = 0;
        glseg->rlights = NULL;
    }
    map->numglsegs = map->numsegs;
}

void BuildGLSubsectors(Map *map)
{
    if (!map->glsubsectors) {
        map->glsubsectors = (subsector_t*)Z_Malloc(map->numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
        for (int i = 0; i < map->numsubsectors; i++) {
            subsector_t *sub = &map->subsectors[i];
            subsector_t *glsub = &map->glsubsectors[i];

            glsub->sector = sub->sector;
            glsub->num_segs = sub->num_segs;
            glsub->first_seg = sub->first_seg;
            glsub->splats = sub->splats;
            glsub->poly = sub->poly;
        }
        map->numglsubsectors = map->numsubsectors;
    }
}

void BuildGLData(Map *map)
{
    CONS_Printf("Building GL data with minisegs...\n");
    BuildGLVertexes(map);
    BuildGLNodes(map);
    CONS_Printf("GL data complete.\n");
}
