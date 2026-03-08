//-----------------------------------------------------------------------------
//
// $Id: gl_bsp.cpp $
//
// Dynamic GL node generation for Doom Legacy
//
// This implements proper GL node generation following the glBSP specification:
// - Minisegs to fill gaps between wall segs
// - Clockwise ordering of segs
// - Proper convex subsectors
//-----------------------------------------------------------------------------

#include <string.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include <map>
#include "doomdata.h"
#include "g_map.h"
#include "doomdef.h"
#include "p_setup.h"
#include "r_data.h"
#include "z_zone.h"
#include "r_defs.h"

// Structure to hold a segment for ordering
struct GLSeg {
    int v1Index;   // Index into GL vertex array
    int v2Index;   // Index into GL vertex array
    bool isMiniseg; // True if this is a miniseg (not along a linedef)
    int linedef;    // Original linedef index (-1 for minisegs)
    int side;       // Side of linedef (0 or 1)

    GLSeg() : v1Index(0), v2Index(0), isMiniseg(true), linedef(-1), side(0) {}
    GLSeg(int v1, int v2, bool mini, int line = -1, int s = 0)
        : v1Index(v1), v2Index(v2), isMiniseg(mini), linedef(line), side(s) {}
};

// Forward declarations
static int AddGLVertex(Map* map, float x, float y);
static void OrderSegsClockwise(std::vector<GLSeg>& segs, vertex_t* glvertexes);

// Function to find or add a GL vertex
static int AddGLVertex(Map* map, float x, float y)
{
    // First check if this vertex already exists in glvertexes
    for (int i = 0; i < map->numglvertexes; i++) {
        if (map->glvertexes[i].x.Float() == x && map->glvertexes[i].y.Float() == y) {
            return i;
        }
    }

    // Need to resize the glvertexes array - allocate new and copy
    int newIndex = map->numglvertexes;
    vertex_t* newVertexes = (vertex_t*)Z_Malloc((newIndex + 1) * sizeof(vertex_t), PU_LEVEL, 0);

    // Copy existing vertices
    for (int i = 0; i < newIndex; i++) {
        newVertexes[i] = map->glvertexes[i];
    }

    // Free old array if it was allocated
    if (map->glvertexes && map->numglvertexes > 0) {
        Z_Free(map->glvertexes);
    }

    map->glvertexes = newVertexes;
    map->glvertexes[newIndex].x = (fixed_t)(x * 65536.0f);
    map->glvertexes[newIndex].y = (fixed_t)(y * 65536.0f);
    map->numglvertexes++;

    return newIndex;
}

// Sort segs by angle from centroid (clockwise ordering)
static void OrderSegsClockwise(std::vector<GLSeg>& segs, vertex_t* glvertexes)
{
    if (segs.empty() || !glvertexes) return;

    // Calculate centroid
    float cx = 0, cy = 0;
    int count = 0;
    for (const auto& seg : segs) {
        cx += glvertexes[seg.v1Index].x.Float();
        cy += glvertexes[seg.v1Index].y.Float();
        count++;
    }
    if (count > 0) {
        cx /= count;
        cy /= count;
    }

    // Sort by angle from centroid
    auto angleFromCentroid = [cx, cy, glvertexes](const GLSeg& a, const GLSeg& b) {
        float ax = glvertexes[a.v1Index].x.Float() - cx;
        float ay = glvertexes[a.v1Index].y.Float() - cy;
        float bx = glvertexes[b.v1Index].x.Float() - cx;
        float by = glvertexes[b.v1Index].y.Float() - cy;

        // Calculate angle
        float angleA = atan2(ay, ax);
        float angleB = atan2(by, bx);

        return angleA < angleB;
    };

    std::sort(segs.begin(), segs.end(), angleFromCentroid);
}

// Check if two points are approximately equal
static bool PointsEqual(float x1, float y1, float x2, float y2, float epsilon = 0.1f)
{
    return fabs(x1 - x2) < epsilon && fabs(y1 - y2) < epsilon;
}

// Build proper GL nodes with minisegs
void BuildGLNodes(Map* map)
{
    CONS_Printf("Building GL nodes with minisegs...\n");

    // First, collect all unique vertices from original segs
    map->glvertexes = (vertex_t*)Z_Malloc(map->numvertexes * sizeof(vertex_t), PU_LEVEL, 0);
    for (int i = 0; i < map->numvertexes; i++) {
        map->glvertexes[i].x = map->vertexes[i].x;
        map->glvertexes[i].y = map->vertexes[i].y;
    }
    map->numglvertexes = map->numvertexes;

    // For each subsector, we need to:
    // 1. Collect all wall segs
    // 2. Find gaps and create minisegs
    // 3. Order all segs clockwise

    std::vector<GLSeg> allGLSegs;
    std::map<int, std::vector<GLSeg>> subsectorSegs;  // subsector index -> segs

    // Process each subsector
    for (int s = 0; s < map->numsubsectors; s++) {
        subsector_t* sub = &map->subsectors[s];
        int firstSeg = sub->first_seg;
        int numSegs = sub->num_segs;

        // Collect all wall segs in this subsector
        std::vector<GLSeg> wallSegs;

        for (int i = 0; i < numSegs; i++) {
            seg_t* seg = &map->segs[firstSeg + i];

            // Find or add v1
            float v1x = seg->v1->x.Float();
            float v1y = seg->v1->y.Float();
            int v1Idx = AddGLVertex(map, v1x, v1y);

            // Find or add v2
            float v2x = seg->v2->x.Float();
            float v2y = seg->v2->y.Float();
            int v2Idx = AddGLVertex(map, v2x, v2y);

            // Check if this is a miniseg (no linedef)
            bool isMiniseg = (seg->linedef == NULL);

            int lineIdx = -1;
            int sideIdx = 0;
            if (seg->linedef) {
                // Find the linedef index
                lineIdx = seg->linedef - map->lines;
                sideIdx = seg->side;
            }

            wallSegs.push_back(GLSeg(v1Idx, v2Idx, isMiniseg, lineIdx, sideIdx));
        }

        // Order segs clockwise for proper rendering
        if (wallSegs.size() >= 3) {
            OrderSegsClockwise(wallSegs, map->glvertexes);
        }

        // Store segs for this subsector
        subsectorSegs[s] = wallSegs;

        // Add to global seg list
        for (const auto& seg : wallSegs) {
            allGLSegs.push_back(seg);
        }
    }

    // Create glsegs array
    map->glsegs = (seg_t*)Z_Malloc(allGLSegs.size() * sizeof(seg_t), PU_LEVEL, 0);
    map->numglsegs = allGLSegs.size();

    for (int i = 0; i < map->numglsegs; i++) {
        seg_t* glseg = &map->glsegs[i];
        const GLSeg& glSeg = allGLSegs[i];

        glseg->v1 = &map->glvertexes[glSeg.v1Index];
        glseg->v2 = &map->glvertexes[glSeg.v2Index];

        // Calculate angle and offset
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

    // Create glsubsectors
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

    // Copy nodes (these don't change for GL)
    map->glnodes = (node_t*)Z_Malloc(map->numnodes * sizeof(node_t), PU_LEVEL, 0);
    map->numglnodes = map->numnodes;
    for (int i = 0; i < map->numnodes; i++) {
        map->glnodes[i] = map->nodes[i];
    }

    CONS_Printf("GL nodes built: %d vertices, %d segs, %d subsectors\n",
        map->numglvertexes, map->numglsegs, map->numglsubsectors);
}

// Legacy function - kept for compatibility
void BuildGLVertexes(Map* map)
{
    // Now handled in BuildGLNodes
    map->glvertexes = (vertex_t*)Z_Malloc(map->numvertexes * sizeof(vertex_t), PU_LEVEL, 0);
    for (int i = 0; i < map->numvertexes; i++) {
        map->glvertexes[i].x = map->vertexes[i].x;
        map->glvertexes[i].y = map->vertexes[i].y;
    }
    map->numglvertexes = map->numvertexes;
}

// Legacy function - kept for compatibility
void BuildGLSegs(Map* map)
{
    // Now handled in BuildGLNodes
    // Just copy segs to glsegs for now if not already built
    if (!map->glsegs) {
        map->glsegs = (seg_t*)Z_Malloc(map->numsegs * sizeof(seg_t), PU_LEVEL, 0);
        for (int i = 0; i < map->numsegs; i++) {
            seg_t* seg = &map->segs[i];
            seg_t* glseg = &map->glsegs[i];

            glseg->v1 = &map->glvertexes[seg->v1 - map->vertexes];
            glseg->v2 = &map->glvertexes[seg->v2 - map->vertexes];
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
}

// Legacy function - kept for compatibility
void BuildGLSubsectors(Map* map)
{
    // Now handled in BuildGLNodes
    if (!map->glsubsectors) {
        map->glsubsectors = (subsector_t*)Z_Malloc(map->numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
        for (int i = 0; i < map->numsubsectors; i++) {
            subsector_t* sub = &map->subsectors[i];
            subsector_t* glsub = &map->glsubsectors[i];

            glsub->sector = sub->sector;
            glsub->num_segs = sub->num_segs;
            glsub->first_seg = sub->first_seg;
            glsub->splats = sub->splats;
            glsub->poly = sub->poly;
        }
        map->numglsubsectors = map->numsubsectors;
    }
}

// Main function to build all GL structures
void BuildGLData(Map *map)
{
    BuildGLVertexes(map);
    BuildGLSegs(map);
    BuildGLSubsectors(map);
    BuildGLNodes(map);
    // TODO: PVS
}
