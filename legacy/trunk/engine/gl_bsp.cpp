//-----------------------------------------------------------------------------
//
// $Id: gl_bsp.cpp $
//
// Dynamic GL node generation for Doom Legacy
//
// Simple version: just copies original BSP data for now
// Full GL node generation with minisegs requires more complex implementation
//-----------------------------------------------------------------------------

#include <string.h>
#include <math.h>
#include "doomdata.h"
#include "g_map.h"
#include "doomdef.h"
#include "p_setup.h"
#include "r_data.h"
#include "z_zone.h"
#include "r_defs.h"

// Function to build GL vertices - simple copy
void BuildGLVertexes(Map *map)
{
    int numglvertexes = map->numvertexes;
    vertex_t *glvertexes = (vertex_t *)Z_Malloc(numglvertexes * sizeof(vertex_t), PU_LEVEL, 0);
    for (int i = 0; i < map->numvertexes; i++)
    {
        // Copy vertex coordinates
        glvertexes[i].x = map->vertexes[i].x;
        glvertexes[i].y = map->vertexes[i].y;
    }
    map->numglvertexes = numglvertexes;
    map->glvertexes = glvertexes;
}

// Function to find partner seg
Uint32 FindPartnerSeg(int segnum, Map *map)
{
    seg_t *seg = &map->segs[segnum];
    if (!seg->linedef)
        return NULL_INDEX_32; // One-sided
    line_t *line = seg->linedef;
    if (!line->sideptr[0] || !line->sideptr[1])
        return NULL_INDEX_32; // One-sided
    // Find the seg on the other side
    for (int i = 0; i < map->numsegs; i++)
    {
        if (map->segs[i].linedef == line && &map->segs[i] != seg)
        {
            return i;
        }
    }
    return NULL_INDEX_32;
}

// Function to build GL segs - simple copy
void BuildGLSegs(Map *map)
{
    int numglsegs = map->numsegs;
    seg_t *glsegs = (seg_t *)Z_Malloc(numglsegs * sizeof(seg_t), PU_LEVEL, 0);
    for (int i = 0; i < map->numsegs; i++)
    {
        seg_t *seg = &map->segs[i];
        seg_t *glseg = &glsegs[i];

        // Point to GL vertices
        if (seg->v1 && seg->v2) {
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

        // Recalculate seg length based on new vertex positions
        float dx = (glseg->v2->x - glseg->v1->x).Float();
        float dy = (glseg->v2->y - glseg->v1->y).Float();
        glseg->length = sqrt(dx * dx + dy * dy);

        glseg->numlights = 0;
        glseg->rlights = NULL;
    }
    map->numglsegs = numglsegs;
    map->glsegs = glsegs;
}

// Function to build GL subsectors - simple copy
void BuildGLSubsectors(Map *map)
{
    int numglsubsectors = map->numsubsectors;
    subsector_t *glsubsectors = (subsector_t *)Z_Malloc(numglsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
    for (int i = 0; i < map->numsubsectors; i++)
    {
        subsector_t *sub = &map->subsectors[i];
        subsector_t *glsub = &glsubsectors[i];

        glsub->sector = sub->sector;
        glsub->num_segs = sub->num_segs;
        glsub->first_seg = sub->first_seg; // Points to glsegs
        glsub->splats = sub->splats;
        glsub->poly = sub->poly;
    }
    map->numglsubsectors = numglsubsectors;
    map->glsubsectors = glsubsectors;
}

// Function to build GL nodes - simple copy
void BuildGLNodes(Map *map)
{
    int numglnodes = map->numnodes;
    node_t *glnodes = (node_t *)Z_Malloc(numglnodes * sizeof(node_t), PU_LEVEL, 0);
    for (int i = 0; i < map->numnodes; i++)
    {
        node_t *node = &map->nodes[i];
        node_t *glnode = &glnodes[i];
        glnode->x = node->x;
        glnode->y = node->y;
        glnode->dx = node->dx;
        glnode->dy = node->dy;
        memcpy(glnode->bbox, node->bbox, sizeof(node->bbox));
        glnode->children[0] = node->children[0];
        glnode->children[1] = node->children[1];
    }
    map->numglnodes = numglnodes;
    map->glnodes = glnodes;
}

// Main function to build all GL structures
void BuildGLData(Map *map)
{
    CONS_Printf("Building GL data (simple copy)...\n");
    BuildGLVertexes(map);
    BuildGLSegs(map);
    BuildGLSubsectors(map);
    BuildGLNodes(map);
    CONS_Printf("GL data built: %d vertices, %d segs, %d subsectors\n",
        map->numglvertexes, map->numglsegs, map->numglsubsectors);
}
