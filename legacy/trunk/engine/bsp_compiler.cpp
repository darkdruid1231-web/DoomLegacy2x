//-----------------------------------------------------------------------------
//
// BSP Node Compiler - Private Implementation
//
// This is the ONLY translation unit that includes ZDBSP headers.
// This completely isolates ZDBSP's TArray, fixed_t, doomdata structs, etc.
// from the rest of the Doom Legacy codebase.
//
// NO Doom Legacy headers allowed in this file!
// All data comes in as plain primitive types via bsp_compiler.h
//
//-----------------------------------------------------------------------------

// Prevent Windows.h macro conflicts
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

// === ALL ZDBSP INCLUDES GO HERE ===
#include "zdbsp.h"           // Main ZDBSP header (includes doomdata.h, workdata.h, etc.)
#include "nodebuild.h"       // FNodeBuilder
#include "tarray.h"          // TArray template

// === NO LEGACY HEADERS HERE ===
// Only our own public header with plain types
#include "bsp_compiler.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

// Implementation structure
struct BspCompilerImpl
{
    std::string errorMessage;

    BspCompilerImpl() {}
    ~BspCompilerImpl() {}
};

BspCompiler::BspCompiler()
    : m_impl(new BspCompilerImpl())
{
}

BspCompiler::~BspCompiler()
{
    delete m_impl;
}

bool BspCompiler::isAvailable()
{
    return true;
}

const char* BspCompiler::getError() const
{
    return m_impl->errorMessage.c_str();
}

bool BspCompiler::buildGLNodes(const char* mapName,
                              const std::vector<ZDBSPInputVertex>& vertices,
                              const std::vector<ZDBSPInputLinedef>& linedefs,
                              const std::vector<ZDBSPInputSidedef>& sidedefs,
                              const std::vector<ZDBSPInputSector>& sectors,
                              ZDBSPOutput& output)
{
    if (!mapName)
    {
        m_impl->errorMessage = "Invalid null argument";
        return false;
    }

    // Reject empty maps - ZDBSP cannot process them
    if (vertices.empty() || linedefs.empty())
    {
        m_impl->errorMessage = "Empty map: no vertices or linedefs";
        return false;
    }

    // Clear any previous output
    output.nodes.clear();
    output.subsectors.clear();
    output.segs.clear();
    output.vertices.clear();

    // Build ZDBSP's FLevel from plain input data
    FLevel level;

    // Convert vertices
    level.NumVertices = (int)vertices.size();
    level.Vertices = new WideVertex[level.NumVertices];
    for (int i = 0; i < (int)vertices.size(); ++i)
    {
        level.Vertices[i].x = vertices[i].x;   // int32_t -> ZDBSP fixed_t (same thing)
        level.Vertices[i].y = vertices[i].y;
        level.Vertices[i].index = i;
    }

    // Convert linedefs to ZDBSP format
    level.Lines.Resize((int)linedefs.size());
    for (int i = 0; i < (int)linedefs.size(); ++i)
    {
        IntLineDef& zld = level.Lines[i];
        zld.v1 = linedefs[i].v1;
        zld.v2 = linedefs[i].v2;
        zld.flags = linedefs[i].flags;
        zld.special = linedefs[i].special;
        // tag is stored in args[0] for binary maps
        zld.args[0] = linedefs[i].tag;
        zld.args[1] = zld.args[2] = zld.args[3] = zld.args[4] = 0;
        zld.sidenum[0] = (linedefs[i].sidenum[0] >= 0) ? (DWORD)linedefs[i].sidenum[0] : 0xFFFFFFFFu;
        zld.sidenum[1] = (linedefs[i].sidenum[1] >= 0) ? (DWORD)linedefs[i].sidenum[1] : 0xFFFFFFFFu;
    }

    // Convert sidedefs
    level.Sides.Resize((int)sidedefs.size());
    for (int i = 0; i < (int)sidedefs.size(); ++i)
    {
        IntSideDef& zsd = level.Sides[i];
        zsd.textureoffset = (short)sidedefs[i].textureoffset;
        zsd.rowoffset = (short)sidedefs[i].rowoffset;
        strncpy(zsd.toptexture, sidedefs[i].toptexture, 8);
        strncpy(zsd.bottomtexture, sidedefs[i].bottomtexture, 8);
        strncpy(zsd.midtexture, sidedefs[i].midtexture, 8);
        zsd.sector = (unsigned)sidedefs[i].sector;
    }

    // Convert sectors
    level.Sectors.Resize((int)sectors.size());
    for (int i = 0; i < (int)sectors.size(); ++i)
    {
        IntSector& zsec = level.Sectors[i];
        zsec.data.floorheight = (short)sectors[i].floorheight;
        zsec.data.ceilingheight = (short)sectors[i].ceilingheight;
        strncpy(zsec.data.floorpic, sectors[i].floorpic, 8);
        strncpy(zsec.data.ceilingpic, sectors[i].ceilingpic, 8);
        zsec.data.lightlevel = (short)sectors[i].lightlevel;
        zsec.data.special = (short)sectors[i].special;
        zsec.data.tag = (short)sectors[i].tag;
    }

    // Find map bounds
    level.FindMapBounds();

    fprintf(stderr, "DEBUG BspCompiler: map=%s vertices=%d linedefs=%d sidedefs=%d sectors=%d\n",
            mapName, (int)vertices.size(), (int)linedefs.size(), (int)sidedefs.size(), (int)sectors.size());
    fprintf(stderr, "DEBUG BspCompiler: level.NumVertices=%d level.Lines.Size()=%d level.MinX=%d MaxX=%d MinY=%d MaxY=%d\n",
            level.NumVertices, (int)level.Lines.Size(), level.MinX, level.MaxX, level.MinY, level.MaxY);
    if (vertices.size() > 0) {
        fprintf(stderr, "DEBUG BspCompiler: vertex[0]=(%d,%d) vertex[1]=(%d,%d)\n",
                level.Vertices[0].x, level.Vertices[0].y,
                level.Vertices[1].x, level.Vertices[1].y);
    }
    if (linedefs.size() > 0) {
        fprintf(stderr, "DEBUG BspCompiler: linedef[0] v1=%u v2=%u sidenum[0]=%u sidenum[1]=%u\n",
                level.Lines[0].v1, level.Lines[0].v2,
                level.Lines[0].sidenum[0], level.Lines[0].sidenum[1]);
    }

    // Build GL nodes using FNodeBuilder
    fprintf(stderr, "DEBUG BspCompiler: Creating FNodeBuilder...\n");
    TArray<FNodeBuilder::FPolyStart> polyspots;
    TArray<FNodeBuilder::FPolyStart> anchors;

    FNodeBuilder builder(level, polyspots, anchors, mapName, true);

    // Get GL nodes - NOTE: These pointers are to FNodeBuilder's internal arrays!
    // We MUST copy the data before builder is destroyed.
    MapNodeEx* nodes = nullptr;
    int nodeCount = 0;
    MapSegGLEx* segs = nullptr;
    int segCount = 0;
    MapSubsectorEx* subs = nullptr;
    int subCount = 0;

    builder.GetGLNodes(nodes, nodeCount, segs, segCount, subs, subCount);

    fprintf(stderr, "DEBUG BspCompiler: GetGLNodes returned: nodes=%d segs=%d subs=%d\n",
            nodeCount, segCount, subCount);
    if (nodes && nodeCount > 0) {
        fprintf(stderr, "DEBUG BspCompiler: first node: x=%d y=%d dx=%d dy=%d children=[0x%08X,0x%08X]\n",
                nodes[0].x, nodes[0].y, nodes[0].dx, nodes[0].dy,
                nodes[0].children[0], nodes[0].children[1]);
    }

    // Get vertices (same deal - internal arrays)
    WideVertex* verts = nullptr;
    int vertCount = 0;
    builder.GetVertices(verts, vertCount);

    fprintf(stderr, "DEBUG BspCompiler: GetVertices returned: vertCount=%d\n", vertCount);
    if (verts && vertCount > 0) {
        fprintf(stderr, "DEBUG BspCompiler: first vertex: x=%d y=%d idx=%d\n",
                verts[0].x, verts[0].y, verts[0].index);
    }

    // NOW copy all the data - this is critical!
    // After this point, builder will go out of scope and internal arrays will be freed

    output.numOriginalVertices = level.NumOrgVerts;

    if (nodes && nodeCount > 0)
    {
        output.nodes.resize(nodeCount);
        memcpy(output.nodes.data(), nodes, nodeCount * sizeof(ZDBSPOutputNode));
    }

    if (subs && subCount > 0)
    {
        output.subsectors.resize(subCount);
        for (int i = 0; i < subCount; ++i)
        {
            output.subsectors[i].numlines = subs[i].numlines;
            output.subsectors[i].firstline = subs[i].firstline;
        }
    }

    if (segs && segCount > 0)
    {
        output.segs.resize(segCount);
        for (int i = 0; i < segCount; ++i)
        {
            output.segs[i].v1 = segs[i].v1;
            output.segs[i].v2 = segs[i].v2;
            output.segs[i].linedef = segs[i].linedef;
            output.segs[i].side = segs[i].side;
            output.segs[i].partner = segs[i].partner;
        }
    }

    if (verts && vertCount > 0)
    {
        output.vertices.resize(vertCount);
        for (int i = 0; i < vertCount; ++i)
        {
            output.vertices[i].x = verts[i].x;    // ZDBSP fixed_t = int, copy directly
            output.vertices[i].y = verts[i].y;
            output.vertices[i].index = verts[i].index;
        }
    }

    return true;
}
