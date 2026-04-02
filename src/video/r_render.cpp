// $Id: r_render.cpp 286 2005-09-29 15:47:55Z smite-meister $
// temporary? Renderer class implementation
// Ville Bergholm

#include "r_render.h"
#include "doomdef.h"
#include "g_map.h"
#include "r_data.h"
#include "r_draw.h"

#include "interfaces/i_render.h"

extern class Map* current_map;

void Rend::SetMap(Map *mp)
{
    m = mp;

    numvertexes = m->numvertexes;
    vertexes = m->vertexes;

    segs = m->segs;

    numsectors = m->numsectors;
    sectors = m->sectors;

    numsubsectors = m->numsubsectors;
    subsectors = m->subsectors;

    numnodes = m->numnodes;
    nodes = m->nodes;

    numlines = m->numlines;
    lines = m->lines;

    sides = m->sides;

    base_colormap = m->fadetable->colormap;
}

//====================================================================
// IRenderBackend adapter - delegates to current_map geometry
//====================================================================

/// \brief Adapter that implements IRenderBackend by delegating to current_map.
/// \details This allows production code to use IRenderBackend pointers while
///         still accessing the real map geometry. In tests, MockRenderBackend
///         can be injected instead.
class RenderBackendAdapter : public IRenderBackend {
public:
    int getVertexCount() const override {
        return current_map ? current_map->numvertexes : 0;
    }

    int getSegCount() const override {
        return current_map ? current_map->numsegs : 0;
    }

    int getSectorCount() const override {
        return current_map ? current_map->numsectors : 0;
    }

    int getSubsectorCount() const override {
        return current_map ? current_map->numsubsectors : 0;
    }

    int getNodeCount() const override {
        return current_map ? current_map->numnodes : 0;
    }

    int getLineCount() const override {
        return current_map ? current_map->numlines : 0;
    }

    const vertex_t* getVertexes() const override {
        return current_map ? current_map->vertexes : nullptr;
    }

    const seg_t* getSegs() const override {
        return current_map ? current_map->segs : nullptr;
    }

    const sector_t* getSectors() const override {
        return current_map ? current_map->sectors : nullptr;
    }

    const subsector_t* getSubsectors() const override {
        return current_map ? current_map->subsectors : nullptr;
    }

    const node_t* getNodes() const override {
        return current_map ? current_map->nodes : nullptr;
    }

    const line_t* getLines() const override {
        return current_map ? current_map->lines : nullptr;
    }

    const side_t* getSides() const override {
        return current_map ? current_map->sides : nullptr;
    }

    bool isSubsectorValid(int index) const override {
        return current_map && index >= 0 && index < current_map->numsubsectors;
    }

    bool isNodeValid(int index) const override {
        return current_map && index >= 0 && index < current_map->numnodes;
    }
};

// Global adapter instance
static RenderBackendAdapter s_renderAdapter;

/// \brief Get the global IRenderBackend instance.
/// \details Returns an adapter pointing to the current map geometry.
///         Production code should use this instead of directly accessing Rend.
IRenderBackend* GetGlobalRenderBackend() {
    return &s_renderAdapter;
}
