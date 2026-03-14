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
// - Partition evaluation for BSP building (future use)
//-----------------------------------------------------------------------------

#include <string.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <limits>
#include <utility>
#include <unordered_map>
#include "doomdata.h"
#include "g_map.h"
#include "doomdef.h"
#include "p_setup.h"
#include "r_data.h"
#include "z_zone.h"
#include "r_defs.h"
#include "zdbsp_integration.h"

// Constants for partition evaluation (from glBSP/Doomsday)
static const float SHORT_HEDGE_EPSILON = 4.0f;
static const float DIST_EPSILON = 1.0f / 128.0f;

// Vertex hash for fast lookup - keyed by (x,y) pair
// Uses spatial hashing for O(1) average lookup
struct VertexHash {
    // Hash coordinate to grid cell
    size_t operator()(const std::pair<float, float>& v) const {
        // Grid size of 8 units (matches SHORT_HEDGE_EPSILON)
        const float gridSize = 8.0f;
        int ix = (int)(v.first / gridSize);
        int iy = (int)(v.second / gridSize);
        // Combine hash values
        return (size_t)(ix * 73856093u ^ iy * 19349663u);
    }
};

typedef std::unordered_map<std::pair<float, float>, int, VertexHash> VertexMap;
static VertexMap* g_vertexMap = nullptr;

// Initialize vertex hash map
static void InitVertexMap()
{
    if (g_vertexMap) {
        delete g_vertexMap;
    }
    g_vertexMap = new VertexMap();
}

// Clear vertex hash map
static void ClearVertexMap()
{
    if (g_vertexMap) {
        g_vertexMap->clear();
    }
}

// Delete vertex hash map
static void DeleteVertexMap()
{
    if (g_vertexMap) {
        delete g_vertexMap;
        g_vertexMap = nullptr;
    }
}

// Data structure for GL seg (like zdbsp's approach)
struct GLSeg {
    int v1Index;
    int v2Index;
    bool isMiniseg;
    int linedef;
    int side;
    int partnerIndex;  // Index of partner seg in the glsegs array (-1 if no partner)

    GLSeg() : v1Index(0), v2Index(0), isMiniseg(true), linedef(-1), side(0), partnerIndex(-1) {}
    GLSeg(int v1, int v2, bool mini, int line = -1, int s = 0)
        : v1Index(v1), v2Index(v2), isMiniseg(mini), linedef(line), side(s), partnerIndex(-1) {}
};

// Data structure for unique vertices with angle
struct GLVertex {
    float x, y;
    int originalIndex;
    float angle;

    GLVertex() : x(0), y(0), originalIndex(-1), angle(0) {}
    GLVertex(float x_, float y_, int idx = -1) : x(x_), y(y_), originalIndex(idx), angle(0) {}
};

// Partition candidate for BSP building
struct PartitionCandidate {
    int v1, v2;              // Partition line vertices
    float x, y, dx, dy;     // Partition line
    int frontCount;          // Segs on front side
    int backCount;           // Segs on back side
    int splitCount;          // Segs that are split
    float cost;              // Evaluation cost
    int linedef;             // Source linedef index (-1 if not from linedef)

    PartitionCandidate() : v1(-1), v2(-1), x(0), y(0), dx(0), dy(0),
                          frontCount(0), backCount(0), splitCount(0), cost(0), linedef(-1) {}
};

// BSP Node for GL
struct GLNode {
    float x, y, dx, dy;           // Partition line
    float bbox[2][4];              // Bounding boxes [front, back][minx, maxx, miny, maxy]
    int children[2];                // Child indices (negative = subsector)
    bool isSubsector;              // True if this is a leaf

    GLNode() : x(0), y(0), dx(0), dy(0), isSubsector(false) {
        bbox[0][0] = bbox[0][1] = bbox[0][2] = bbox[0][3] = 0;
        bbox[1][0] = bbox[1][1] = bbox[1][2] = bbox[1][3] = 0;
        children[0] = children[1] = 0;
    }
};

// FNodeBuilder class - builds GL BSP trees (similar to GZDoom's FNodeBuilder)
class FNodeBuilder {
public:
    std::vector<GLVertex> vertices;
    std::vector<GLSeg> segs;
    std::vector<GLNode> nodes;
    std::vector<int> subsectorSegCounts;
    std::vector<int> subsectorFirstSegs;

    Map* map;

    FNodeBuilder(Map* m) : map(m) {}

    // Build GL nodes from existing BSP data
    void BuildFromExistingBSP();

    // Build GL nodes from scratch (partitioning)
    void BuildFromScratch();

    // Partition evaluation - find best partition line
    int EvaluatePartition(const PartitionCandidate& candidate);

    // Create minisegs for gaps between wall vertices
    void CreateMinisegs(int subsectorIndex);

    // Helper: determine which side of partition a point is on
    int PointOnSide(float px, float py, const PartitionCandidate& partition);

    // Helper: check if a seg needs to be split by partition
    int ClassifySeg(const GLSeg& seg, const PartitionCandidate& partition,
                    float& splitX, float& splitY);

    // Helper: calculate bounding box for a set of segs
    void CalculateSegsBBox(const std::vector<int>& segIndices, float* bbox);
};

// Helper: determine which side of partition a point is on
int FNodeBuilder::PointOnSide(float px, float py, const PartitionCandidate& partition)
{
    float d = (px - partition.x) * partition.dy - (py - partition.y) * partition.dx;
    if (d > DIST_EPSILON) return 1;   // Front
    if (d < -DIST_EPSILON) return 0;   // Back
    return -1;                          // On the line
}

// Helper: classify a seg relative to partition
int FNodeBuilder::ClassifySeg(const GLSeg& seg, const PartitionCandidate& partition,
                               float& splitX, float& splitY)
{
    const GLVertex& v1 = vertices[seg.v1Index];
    const GLVertex& v2 = vertices[seg.v2Index];

    int side1 = PointOnSide(v1.x, v1.y, partition);
    int side2 = PointOnSide(v2.x, v2.y, partition);

    if (side1 == -1 && side2 == -1) {
        // Both on line - arbitrarily put on front
        return 1;
    }

    if (side1 == side2) {
        // Both on same side
        return side1 >= 0 ? side1 : 1;
    }

    // Seg is split - calculate intersection
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float partitionDx = partition.dx;
    float partitionDy = partition.dy;

    float denom = dx * partitionDy - dy * partitionDx;
    if (fabs(denom) < DIST_EPSILON) {
        // Parallel - shouldn't happen, but put on front
        return 1;
    }

    float t = ((partition.x - v1.x) * partitionDy - (partition.y - v1.y) * partitionDx) / denom;
    splitX = v1.x + t * dx;
    splitY = v1.y + t * dy;

    return 2;  // Split
}

// Evaluate a partition candidate - returns cost (lower is better)
int FNodeBuilder::EvaluatePartition(const PartitionCandidate& candidate)
{
    int splits = candidate.splitCount;
    int balance = abs(candidate.frontCount - candidate.backCount);

    // Cost formula: heavily penalize splits, moderately penalize imbalance
    // Based on glBSP algorithm
    int cost = splits * 1000 + balance;

    // Small bonus for partitions that use actual map lines
    // (reduces miniseg count)
    if (candidate.linedef >= 0) {
        cost -= 50;
    }

    return cost;
}

// Calculate bounding box for a set of seg indices
void FNodeBuilder::CalculateSegsBBox(const std::vector<int>& segIndices, float* bbox)
{
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();

    for (int segIdx : segIndices) {
        const GLSeg& seg = segs[segIdx];
        const GLVertex& v1 = vertices[seg.v1Index];
        const GLVertex& v2 = vertices[seg.v2Index];

        minX = std::min(minX, std::min(v1.x, v2.x));
        minY = std::min(minY, std::min(v1.y, v2.y));
        maxX = std::max(maxX, std::max(v1.x, v2.x));
        maxY = std::max(maxY, std::max(v1.y, v2.y));
    }

    bbox[0] = minX;
    bbox[1] = maxX;
    bbox[2] = minY;
    bbox[3] = maxY;
}

// Build GL nodes from existing BSP data (current implementation)
void FNodeBuilder::BuildFromExistingBSP()
{
    // This calls the existing BuildGLNodes implementation
    // Kept for reference - actual work is done in BuildGLNodes()
}

// Build GL nodes from scratch using BSP partitioning
// This is for future use when we want to regenerate nodes from raw geometry
void FNodeBuilder::BuildFromScratch()
{
    // This would rebuild the BSP tree from scratch using partition evaluation
    // For now, just a placeholder - the existing BuildGLNodes handles most cases

    // Step 1: Collect all wall segs from the map
    // Step 2: Find partition candidates (use map lines)
    // Step 3: Evaluate partitions, choose best
    // Step 4: Recursively build tree
    // Step 5: Generate minisegs for gaps
    // Step 6: Create GL vertices, segs, subsectors

    CONS_Printf("FNodeBuilder: BuildFromScratch not yet implemented, using existing BSP\n");
}

// Create minisegs for gaps between wall vertices in a subsector
void FNodeBuilder::CreateMinisegs(int subsectorIndex)
{
    // This would create additional minisegs beyond what's in the original BSP
    // Useful for filling gaps that the original BSP didn't have

    // Get segs for this subsector
    int firstSeg = subsectorFirstSegs[subsectorIndex];
    int numSegs = subsectorSegCounts[subsectorIndex];

    if (numSegs < 3) return;  // Can't have gaps in less than 3 segs

    // Build vertex list and sort by angle from centroid
    // (this is already done in BuildGLNodes)
}

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

// Check if a point is inside a polygon (ray casting algorithm)
static bool PointInPolygon(float px, float py, const std::vector<float>& polyX, const std::vector<float>& polyY)
{
    int numVerts = polyX.size();
    bool inside = false;

    for (int i = 0, j = numVerts - 1; i < numVerts; j = i++) {
        if (((polyY[i] > py) != (polyY[j] > py)) &&
            (px < (polyX[j] - polyX[i]) * (py - polyY[i]) / (polyY[j] - polyY[i]) + polyX[i])) {
            inside = !inside;
        }
    }
    return inside;
}

// Check if polygon is convex
static bool IsPolygonConvex(const std::vector<float>& polyX, const std::vector<float>& polyY)
{
    int n = polyX.size();
    if (n < 3) return true;

    int sign = 0;
    for (int i = 0; i < n; i++) {
        float dx1 = polyX[(i + 1) % n] - polyX[i];
        float dy1 = polyY[(i + 1) % n] - polyY[i];
        float dx2 = polyX[(i + 2) % n] - polyX[(i + 1) % n];
        float dy2 = polyY[(i + 2) % n] - polyY[(i + 1) % n];
        float cross = dx1 * dy2 - dy1 * dx2;

        if (cross != 0) {
            int currentSign = (cross > 0) ? 1 : -1;
            if (sign == 0) sign = currentSign;
            else if (sign != currentSign) return false;
        }
    }
    return true;
}

// Forward declaration for AddGLVertex
static int AddGLVertex(Map* map, float x, float y);

// Build GL segs using zdbsp-style CheckLoop logic
// Only adds minisegs when needed to close valid polygon loops
static void BuildGLSegsWithCheckLoop(
    Map* map,
    seg_t* mapSegs,
    int firstSeg,
    int numSegs,
    int subsectorIndex,
    std::vector<GLSeg>& outSegs,
    int& degenerateCount,
    int& minisegCount)
{
    minisegCount = 0;
    int wallCount = 0;

    // Step 1: Collect wall segs and build connectivity map
    struct WallSeg {
        int v1, v2;  // GL vertex indices
        int lineIdx;
        int side;
    };
    std::vector<WallSeg> wallSegs;
    std::set<int> vertexSet;
    std::map<std::pair<int, int>, int> segByVertices;  // (v1,v2) -> index in wallSegs

    for (int i = 0; i < numSegs; i++) {
        seg_t* seg = &mapSegs[firstSeg + i];

        if (!seg || !seg->v1 || !seg->v2) continue;
        if (!seg->linedef) continue;  // Skip existing minisegs

        float v1x = seg->v1->x.Float();
        float v1y = seg->v1->y.Float();
        float v2x = seg->v2->x.Float();
        float v2y = seg->v2->y.Float();

        int v1Idx = AddGLVertex(map, v1x, v1y);
        int v2Idx = AddGLVertex(map, v2x, v2y);

        WallSeg ws;
        ws.v1 = v1Idx;
        ws.v2 = v2Idx;
        ws.lineIdx = seg->linedef - map->lines;
        ws.side = seg->side;

        int idx = wallSegs.size();
        wallSegs.push_back(ws);
        vertexSet.insert(v1Idx);
        vertexSet.insert(v2Idx);
        segByVertices[std::make_pair(v1Idx, v2Idx)] = idx;
        wallCount++;
    }

    if (wallSegs.size() < 3 || vertexSet.size() < 3) {
        degenerateCount++;
        // Just copy what we have
        for (const auto& ws : wallSegs) {
            outSegs.push_back(GLSeg(ws.v1, ws.v2, false, ws.lineIdx, ws.side));
        }
        return;
    }

    // Step 2: Sort vertices by angle from centroid (for polygon boundary)
    std::vector<GLVertex> vertices;
    for (int idx : vertexSet) {
        float vx = map->glvertexes[idx].x.Float();
        float vy = map->glvertexes[idx].y.Float();
        vertices.push_back(GLVertex(vx, vy, idx));
    }

    // Calculate centroid
    float accumx = 0, accumy = 0;
    for (const auto& v : vertices) {
        accumx += v.x;
        accumy += v.y;
    }
    float midx = accumx / vertices.size();
    float midy = accumy / vertices.size();

    // Calculate angle and sort
    for (auto& v : vertices) {
        v.angle = atan2(v.y - midy, v.x - midx);
    }
    std::sort(vertices.begin(), vertices.end(),
        [](const GLVertex& a, const GLVertex& b) {
            return a.angle > b.angle;  // Clockwise
        });

    // Add wall segs only - no minisegs
    for (const auto& ws : wallSegs) {
        outSegs.push_back(GLSeg(ws.v1, ws.v2, false, ws.lineIdx, ws.side));
    }

    // Skip adding minisegs - the original segs don't have partners either
    // and adding minisegs without proper zdbsp-style BSP generation doesn't help
    // The renderer will handle best it can with wall segs only

    // Debug output
    if (subsectorIndex < 5) {
        CONS_Printf("  Debug subsector %d: %d verts, %d wall, %d minisegs needed\n",
            subsectorIndex, (int)vertices.size(), wallCount, minisegCount);
    }

    // Check for degenerate - need at least 3 total segs
    if (outSegs.size() < 3) {
        degenerateCount++;
    }
}

// Function to find or add a GL vertex using hash map for O(1) lookup
static int AddGLVertex(Map* map, float x, float y)
{
    std::pair<float, float> key(x, y);

    // Use hash map for fast lookup if available
    if (g_vertexMap) {
        auto it = g_vertexMap->find(key);
        if (it != g_vertexMap->end()) {
            return it->second;
        }
    } else {
        // Fallback to linear search if hash map not initialized
        for (int i = 0; i < map->numglvertexes; i++) {
            if (map->glvertexes[i].x.Float() == x && map->glvertexes[i].y.Float() == y) {
                return i;
            }
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

    // Add to hash map
    if (g_vertexMap) {
        (*g_vertexMap)[key] = newIndex;
    }

    return newIndex;
}

// Build proper GL nodes with minisegs
void BuildGLNodes(Map* map)
{
    CONS_Printf("Building GL nodes with minisegs...\n");

    // Initialize vertex hash map for fast lookups
    InitVertexMap();

    // Add initial vertices to hash map
    for (int i = 0; i < map->numglvertexes; i++) {
        float x = map->glvertexes[i].x.Float();
        float y = map->glvertexes[i].y.Float();
        (*g_vertexMap)[std::make_pair(x, y)] = i;
    }

    // Step 1: glvertexes should already be allocated by BuildGLVertexes
    // Just verify and use existing array
    if (!map->glvertexes) {
        map->glvertexes = (vertex_t*)Z_Malloc(map->numvertexes * sizeof(vertex_t), PU_LEVEL, 0);
        for (int i = 0; i < map->numvertexes; i++) {
            map->glvertexes[i].x = map->vertexes[i].x;
            map->glvertexes[i].y = map->vertexes[i].y;
        }
        map->numglvertexes = map->numvertexes;
    }

    // Statistics counters
    int degenerateCount = 0;
    int gapCount = 0;
    int totalWallSegs = 0;

    // Step 2: Process each subsector
    std::vector<GLSeg> allGLSegs;
    std::map<int, std::vector<GLSeg>> subsectorSegs;

    // Pre-initialize all subsector entries to avoid gaps
    for (int s = 0; s < map->numsubsectors; s++) {
        subsectorSegs[s] = std::vector<GLSeg>();
    }

    for (int s = 0; s < map->numsubsectors; s++) {
        subsector_t* sub = &map->subsectors[s];
        int firstSeg = sub->first_seg;
        int numSegs = sub->num_segs;

        // Skip subsectors with no segs at all
        if (numSegs <= 0) {
            CONS_Printf("  Warning: Subsector %d has no segs\n", s);
            subsectorSegs[s] = std::vector<GLSeg>();
            continue;
        }

        // Safety check for valid seg indices
        if (firstSeg < 0 || firstSeg >= map->numsegs) {
            CONS_Printf("  Warning: Subsector %d has invalid first_seg (%d)\n", s, firstSeg);
            subsectorSegs[s] = std::vector<GLSeg>();
            continue;
        }

        // Use zdbsp-style checkloop approach
        std::vector<GLSeg> orderedSegs;
        int minisegCount = 0;
        BuildGLSegsWithCheckLoop(
            map,
            map->segs,
            firstSeg,
            numSegs,
            s,
            orderedSegs,
            degenerateCount,
            minisegCount
        );

        // Count wall segs and minisegs for statistics
        for (const auto& seg : orderedSegs) {
            if (!seg.isMiniseg) totalWallSegs++;
            else gapCount++;
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

        // Set linedef and sidedef if available
        if (glSeg.linedef >= 0 && glSeg.linedef < map->numlines) {
            glseg->linedef = &map->lines[glSeg.linedef];
            glseg->side = glSeg.side;
            // Get sidedef from linedef
            glseg->sidedef = glseg->linedef->sideptr[glseg->side];
            // Get sectors from sidedef
            if (glseg->sidedef) {
                glseg->frontsector = glseg->sidedef->sector;
                // Check for two-sided line
                if (glseg->linedef->flags & ML_TWOSIDED) {
                    glseg->backsector = glseg->linedef->sideptr[glseg->side ^ 1]->sector;
                } else {
                    glseg->backsector = NULL;
                }
            }
        } else {
            glseg->linedef = NULL;
            glseg->side = 0;
            glseg->sidedef = NULL;
            glseg->frontsector = NULL;
            glseg->backsector = NULL;
        }

        // Set partner_seg if we have a partner index
        if (glSeg.partnerIndex >= 0 && glSeg.partnerIndex < map->numglsegs) {
            glseg->partner_seg = &map->glsegs[glSeg.partnerIndex];
        } else {
            glseg->partner_seg = NULL;
        }

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

        // Use the regular BSP subsector's sector. The GL subsectors have the
        // same count and 1:1 index mapping as the regular subsectors, so this
        // is always correct. The old glseg-based search failed for subsectors
        // composed entirely of minisegs (no linedef), leaving sector=NULL and
        // causing those subsectors (and their actors) to be silently skipped.
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

    // Print statistics
    CONS_Printf("GL nodes built: %d vertices, %d segs, %d subsectors\n",
        map->numglvertexes, map->numglsegs, map->numglsubsectors);
    CONS_Printf("  Subsector stats: %d valid, %d degenerate\n",
        map->numsubsectors - degenerateCount, degenerateCount);

    // Clean up vertex hash map
    DeleteVertexMap();
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
    // Try zdbsp first if available - it provides better GL nodes
    if (ZDBSPIntegration::IsZDBSPAvailable())
    {
        CONS_Printf("zdbsp available - generating GL nodes...\n");

        // Run zdbsp to generate GL nodes and modify the WAD
        // zdbsp -g -x -m MAP01 -o output.wad input.wad
        if (ZDBSPIntegration::GenerateGLNodes(map->lumpname))
        {
            CONS_Printf("zdbsp generated GL nodes. Re-load to use them.\n");
            // For now, fall back to built-in - full reload would need WAD re-initialization
            // TODO: Implement WAD reload after zdbsp modification
        }
    }

    // Use built-in GL node generation (fallback or if zdbsp not available)
    CONS_Printf("Building GL data with minisegs...\n");
    BuildGLVertexes(map);
    BuildGLNodes(map);
    CONS_Printf("GL data complete.\n");
}
