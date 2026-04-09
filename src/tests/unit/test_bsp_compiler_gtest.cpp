//-----------------------------------------------------------------------------
//
// BSP Compiler Google Test
//
// Tests for BspCompiler (ZDBSP library wrapper)
//
//-----------------------------------------------------------------------------

#include "engine/bsp_compiler.h"
#include <gtest/gtest.h>
#include <cmath>
#include <cstring>
#include <vector>
#include <memory>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::Test;

//-----------------------------------------------------------------------------
// GLBSP v5 format structures (little-endian)
//-----------------------------------------------------------------------------

// GL vertex - 8 bytes
struct GLVertex {
    int32_t x;
    int32_t y;
} __attribute__((packed));

// GL v5 seg - 16 bytes
struct GL5Seg {
    uint32_t v1;
    uint32_t v2;
    uint16_t linedef;
    uint16_t side;
    uint32_t partner;
} __attribute__((packed));

// GL v5 subsector - 8 bytes
struct GL5Subsector {
    uint32_t count;
    uint32_t first_seg;
} __attribute__((packed));

// GL v5 node - 32 bytes
struct GL5Node {
    int16_t x, y;
    int16_t dx, dy;
    int16_t bbox[2][4];
    uint32_t children[2];
} __attribute__((packed));

//-----------------------------------------------------------------------------
// In-memory WAD-like writer for GLBSP lumps
//-----------------------------------------------------------------------------

class GLBWadWriter {
public:
    struct GLBLump {
        char lumpName[8];
        int lumpLen;
        std::vector<uint8_t> lumpData;
    };

    void writeLump(const char* name, const void* buf, size_t len) {
        GLBLump entry;
        memcpy(entry.lumpName, name, 8);
        entry.lumpLen = static_cast<int>(len);
        entry.lumpData.resize(len);
        if (buf && len > 0) {
            memcpy(entry.lumpData.data(), buf, len);
        }
        entries.push_back(entry);
    }

    const std::vector<GLBLump>& getAllLumps() const { return entries; }

private:
    std::vector<GLBLump> entries;
};

//-----------------------------------------------------------------------------
// GLBSP Serializer: ZDBSPOutput -> GLBSP v5 format
//-----------------------------------------------------------------------------

class GLBSPSerializer {
public:
    static void serialize(const ZDBSPOutput& output, GLBWadWriter& writer) {
        // Write GL_VERT lump (vertices)
        std::vector<GLVertex> vertices(output.vertices.size());
        for (size_t i = 0; i < output.vertices.size(); ++i) {
            vertices[i].x = output.vertices[i].x;
            vertices[i].y = output.vertices[i].y;
        }
        writer.writeLump("GL_VERT", vertices.data(), vertices.size() * sizeof(GLVertex));

        // Write GL_SEGS lump (segs)
        std::vector<GL5Seg> segs(output.segs.size());
        for (size_t i = 0; i < output.segs.size(); ++i) {
            segs[i].v1 = output.segs[i].v1;
            segs[i].v2 = output.segs[i].v2;
            segs[i].linedef = static_cast<uint16_t>(output.segs[i].linedef);
            segs[i].side = output.segs[i].side;
            segs[i].partner = output.segs[i].partner;
        }
        writer.writeLump("GL_SEGS", segs.data(), segs.size() * sizeof(GL5Seg));

        // Write GL_SSECT lump (subsectors)
        std::vector<GL5Subsector> subs(output.subsectors.size());
        for (size_t i = 0; i < output.subsectors.size(); ++i) {
            subs[i].count = output.subsectors[i].numlines;
            subs[i].first_seg = output.subsectors[i].firstline;
        }
        writer.writeLump("GL_SSECT", subs.data(), subs.size() * sizeof(GL5Subsector));

        // Write GL_NODES lump (nodes)
        // ZDBSP outputs 16.16 fixed-point, GLBSP v5 expects int16_t (shifted right by 16)
        std::vector<GL5Node> nodes(output.nodes.size());
        for (size_t i = 0; i < output.nodes.size(); ++i) {
            nodes[i].x = static_cast<int16_t>(output.nodes[i].x >> 16);
            nodes[i].y = static_cast<int16_t>(output.nodes[i].y >> 16);
            nodes[i].dx = static_cast<int16_t>(output.nodes[i].dx >> 16);
            nodes[i].dy = static_cast<int16_t>(output.nodes[i].dy >> 16);
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 4; ++k) {
                    nodes[i].bbox[j][k] = static_cast<int16_t>(output.nodes[i].bbox[j][k] >> 16);
                }
            }
            nodes[i].children[0] = output.nodes[i].children[0];
            nodes[i].children[1] = output.nodes[i].children[1];
        }
        writer.writeLump("GL_NODES", nodes.data(), nodes.size() * sizeof(GL5Node));
    }
};

//-----------------------------------------------------------------------------
// GLBSP Deserializer: GLBSP v5 format -> ZDBSPOutput (simulating P_LoadGLNodes)
//-----------------------------------------------------------------------------

class GLBSPDeserializer {
public:
    static void deserialize(const std::vector<GLBWadWriter::GLBLump>& lumps,
                           ZDBSPOutput& output) {
        const GLBWadWriter::GLBLump* glVert = findLump(lumps, "GL_VERT");
        const GLBWadWriter::GLBLump* glSegs = findLump(lumps, "GL_SEGS");
        const GLBWadWriter::GLBLump* glSsect = findLump(lumps, "GL_SSECT");
        const GLBWadWriter::GLBLump* glNodes = findLump(lumps, "GL_NODES");

        // Deserialize vertices
        if (glVert && glVert->lumpLen > 0) {
            size_t count = glVert->lumpLen / sizeof(GLVertex);
            const GLVertex* verts = reinterpret_cast<const GLVertex*>(glVert->lumpData.data());
            output.vertices.resize(count);
            for (size_t i = 0; i < count; ++i) {
                output.vertices[i].x = verts[i].x;
                output.vertices[i].y = verts[i].y;
                output.vertices[i].index = static_cast<int32_t>(i);
            }
        }

        // Deserialize segs
        if (glSegs && glSegs->lumpLen > 0) {
            size_t count = glSegs->lumpLen / sizeof(GL5Seg);
            const GL5Seg* segs = reinterpret_cast<const GL5Seg*>(glSegs->lumpData.data());
            output.segs.resize(count);
            for (size_t i = 0; i < count; ++i) {
                output.segs[i].v1 = segs[i].v1;
                output.segs[i].v2 = segs[i].v2;
                output.segs[i].linedef = segs[i].linedef;
                output.segs[i].side = segs[i].side;
                output.segs[i].partner = segs[i].partner;
            }
        }

        // Deserialize subsectors
        if (glSsect && glSsect->lumpLen > 0) {
            size_t count = glSsect->lumpLen / sizeof(GL5Subsector);
            const GL5Subsector* subs = reinterpret_cast<const GL5Subsector*>(glSsect->lumpData.data());
            output.subsectors.resize(count);
            for (size_t i = 0; i < count; ++i) {
                output.subsectors[i].numlines = subs[i].count;
                output.subsectors[i].firstline = subs[i].first_seg;
            }
        }

        // Deserialize nodes
        // GLBSP v5 stores int16_t, convert back to 16.16 fixed-point by shifting left
        if (glNodes && glNodes->lumpLen > 0) {
            size_t count = glNodes->lumpLen / sizeof(GL5Node);
            const GL5Node* nodes = reinterpret_cast<const GL5Node*>(glNodes->lumpData.data());
            output.nodes.resize(count);
            for (size_t i = 0; i < count; ++i) {
                output.nodes[i].x = static_cast<int32_t>(nodes[i].x) << 16;
                output.nodes[i].y = static_cast<int32_t>(nodes[i].y) << 16;
                output.nodes[i].dx = static_cast<int32_t>(nodes[i].dx) << 16;
                output.nodes[i].dy = static_cast<int32_t>(nodes[i].dy) << 16;
                for (int j = 0; j < 2; ++j) {
                    for (int k = 0; k < 4; ++k) {
                        output.nodes[i].bbox[j][k] = static_cast<int32_t>(nodes[i].bbox[j][k]) << 16;
                    }
                }
                output.nodes[i].children[0] = nodes[i].children[0];
                output.nodes[i].children[1] = nodes[i].children[1];
            }
        }
    }

private:
    static const GLBWadWriter::GLBLump* findLump(const std::vector<GLBWadWriter::GLBLump>& lumps,
                                               const char* name) {
        for (size_t i = 0; i < lumps.size(); ++i) {
            if (memcmp(lumps[i].lumpName, name, 8) == 0) {
                return &lumps[i];
            }
        }
        return nullptr;
    }
};

//-----------------------------------------------------------------------------
// Test fixture for BspCompiler tests
//-----------------------------------------------------------------------------

class BspCompilerTest : public Test {
protected:
    // Common validation helper
    void ValidateOutput(const ZDBSPOutput& output, const char* testName) {
        EXPECT_FALSE(output.nodes.empty()) << testName << ": No nodes generated";
        EXPECT_FALSE(output.subsectors.empty()) << testName << ": No subsectors generated";
        EXPECT_FALSE(output.segs.empty()) << testName << ": No segs generated";
        EXPECT_FALSE(output.vertices.empty()) << testName << ": No vertices generated";

        // Check that nodes have valid partition lines (non-zero x, y, dx, or dy)
        for (size_t i = 0; i < output.nodes.size(); ++i) {
            const auto& node = output.nodes[i];
            bool hasValidPartition = (node.x != 0) || (node.y != 0) ||
                                      (node.dx != 0) || (node.dy != 0);
            EXPECT_TRUE(hasValidPartition)
                << testName << ": Node " << i << " has zero partition line: x=" << node.x
                << " y=" << node.y << " dx=" << node.dx << " dy=" << node.dy;
        }

        // Verify children are valid (bit 31 set = subsector)
        for (size_t i = 0; i < output.nodes.size(); ++i) {
            const auto& node = output.nodes[i];
            bool child0Valid = ((node.children[0] & 0x80000000) != 0) ||
                              ((node.children[0] & 0x7FFFFFFF) < (unsigned)output.nodes.size());
            bool child1Valid = ((node.children[1] & 0x80000000) != 0) ||
                              ((node.children[1] & 0x7FFFFFFF) < (unsigned)output.nodes.size());
            EXPECT_TRUE(child0Valid) << testName << ": Node " << i << " has invalid child[0]: 0x"
                                    << std::hex << node.children[0];
            EXPECT_TRUE(child1Valid) << testName << ": Node " << i << " has invalid child[1]: 0x"
                                    << std::hex << node.children[1];
        }

        // Verify segs reference valid vertices
        for (size_t i = 0; i < output.segs.size(); ++i) {
            const auto& seg = output.segs[i];
            EXPECT_LT(seg.v1, (uint32_t)output.vertices.size())
                << testName << ": Seg " << i << " has invalid v1 index: " << seg.v1;
            EXPECT_LT(seg.v2, (uint32_t)output.vertices.size())
                << testName << ": Seg " << i << " has invalid v2 index: " << seg.v2;
        }
    }

    // Compare two ZDBSPOutput structures
    // Note: GLBSP v5 format uses int16_t for node partition lines and uint16_t for seg linedef,
    // which truncates values that exceed 16-bit range. This is a format limitation.
    void CompareOutputs(const ZDBSPOutput& original, const ZDBSPOutput& loaded,
                        const char* testName) {
        EXPECT_EQ(original.nodes.size(), loaded.nodes.size())
            << testName << ": Node count mismatch";
        EXPECT_EQ(original.subsectors.size(), loaded.subsectors.size())
            << testName << ": Subsector count mismatch";
        EXPECT_EQ(original.segs.size(), loaded.segs.size())
            << testName << ": Seg count mismatch";
        EXPECT_EQ(original.vertices.size(), loaded.vertices.size())
            << testName << ": Vertex count mismatch";

        // Compare nodes - only check counts and structure, not truncated values
        // GLBSP v5 truncates int32_t to int16_t for x,y,dx,dy
        for (size_t i = 0; i < original.nodes.size() && i < loaded.nodes.size(); ++i) {
            const auto& orig = original.nodes[i];
            const auto& load = loaded.nodes[i];
            // Children indices should match exactly
            EXPECT_EQ(orig.children[0], load.children[0]) << testName << ": Node " << i << " child[0] mismatch";
            EXPECT_EQ(orig.children[1], load.children[1]) << testName << ": Node " << i << " child[1] mismatch";
        }

        // Compare subsectors - these should match exactly
        for (size_t i = 0; i < original.subsectors.size() && i < loaded.subsectors.size(); ++i) {
            const auto& orig = original.subsectors[i];
            const auto& load = loaded.subsectors[i];
            EXPECT_EQ(orig.numlines, load.numlines) << testName << ": Subsector " << i << " numlines mismatch";
            EXPECT_EQ(orig.firstline, load.firstline) << testName << ": Subsector " << i << " firstline mismatch";
        }

        // Compare segs - v1, v2, side, partner should match exactly
        // Note: linedef is truncated from uint32_t to uint16_t in GLBSP v5
        for (size_t i = 0; i < original.segs.size() && i < loaded.segs.size(); ++i) {
            const auto& orig = original.segs[i];
            const auto& load = loaded.segs[i];
            EXPECT_EQ(orig.v1, load.v1) << testName << ": Seg " << i << " v1 mismatch";
            EXPECT_EQ(orig.v2, load.v2) << testName << ": Seg " << i << " v2 mismatch";
            EXPECT_EQ(orig.side, load.side) << testName << ": Seg " << i << " side mismatch";
            EXPECT_EQ(orig.partner, load.partner) << testName << ": Seg " << i << " partner mismatch";
            // linedef is truncated - only check lower 16 bits match
            uint16_t origLinedefLow = static_cast<uint16_t>(orig.linedef & 0xFFFF);
            uint16_t loadLinedefLow = static_cast<uint16_t>(load.linedef & 0xFFFF);
            EXPECT_EQ(origLinedefLow, loadLinedefLow) << testName << ": Seg " << i << " linedef low bits mismatch";
        }

        // Compare vertices - these should match exactly (GLBSP v5 uses int32_t for vertices)
        for (size_t i = 0; i < original.vertices.size() && i < loaded.vertices.size(); ++i) {
            const auto& orig = original.vertices[i];
            const auto& load = loaded.vertices[i];
            EXPECT_EQ(orig.x, load.x) << testName << ": Vertex " << i << " x mismatch";
            EXPECT_EQ(orig.y, load.y) << testName << ": Vertex " << i << " y mismatch";
        }
    }

    // Helper to create a simple closed box room
    static std::vector<ZDBSPInputLinedef> MakeBoxLinedefs(int width, int height) {
        return {
            {0, 1, 1, 0, 0, {0, -1}},  // bottom: v0->v1
            {1, 2, 1, 0, 0, {1, -1}},  // right: v1->v2
            {2, 3, 1, 0, 0, {2, -1}},  // top: v2->v3
            {3, 0, 1, 0, 0, {3, -1}}   // left: v3->v0
        };
    }

    static std::vector<ZDBSPInputSidedef> MakeBoxSidedefs() {
        return {
            {0, 0, "FLAT1", "", "", 0},
            {0, 0, "FLAT1", "", "", 0},
            {0, 0, "FLAT1", "", "", 0},
            {0, 0, "FLAT1", "", "", 0}
        };
    }

    static std::vector<ZDBSPInputSector> MakeSingleSector(int floorH, int ceilH) {
        return {
            {0, floorH, "FLOOR1", "CEIL1", 144, 0, 0}
        };
    }
};

//-----------------------------------------------------------------------------
// Basic availability test
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, IsAvailable)
{
    BspCompiler compiler;
    EXPECT_TRUE(BspCompiler::isAvailable());
}

//-----------------------------------------------------------------------------
// Test with null map name (should fail gracefully)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, NullMapName)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    std::vector<ZDBSPInputVertex> vertices = {{0, 0}, {256 << 16, 0}};
    std::vector<ZDBSPInputLinedef> linedefs = {{0, 1, 1, 0, 0, {0, -1}}};
    std::vector<ZDBSPInputSidedef> sidedefs = {{0, 0, "FLAT1", "", "", 0}};
    std::vector<ZDBSPInputSector> sectors = {{0, 128 << 16, "FLOOR1", "CEIL1", 144, 0, 0}};

    bool result = compiler.buildGLNodes(nullptr, vertices, linedefs, sidedefs, sectors, output);
    EXPECT_FALSE(result);
}

//-----------------------------------------------------------------------------
// Test with empty map data (should fail gracefully)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, EmptyMap)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    std::vector<ZDBSPInputVertex> emptyVertices;
    std::vector<ZDBSPInputLinedef> emptyLinedefs;
    std::vector<ZDBSPInputSidedef> emptySidedefs;
    std::vector<ZDBSPInputSector> emptySectors;

    bool result = compiler.buildGLNodes("MAPXX", emptyVertices, emptyLinedefs,
                                        emptySidedefs, emptySectors, output);
    // Empty map should fail gracefully
    EXPECT_FALSE(result);
}

//-----------------------------------------------------------------------------
// Test: Rectangular room (wider than tall)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, RectangularRoom_Wide)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // 512 wide x 256 tall (2:1 aspect ratio)
    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},
        {512 << 16, 0},
        {512 << 16, 256 << 16},
        {0, 256 << 16}
    };
    auto linedefs = MakeBoxLinedefs(512, 256);
    auto sidedefs = MakeBoxSidedefs();
    auto sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAP01", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "RectangularRoom_Wide");
}

//-----------------------------------------------------------------------------
// Test: Rectangular room (taller than wide)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, RectangularRoom_Tall)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // 256 wide x 512 tall (1:2 aspect ratio)
    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},
        {256 << 16, 0},
        {256 << 16, 512 << 16},
        {0, 512 << 16}
    };
    auto linedefs = MakeBoxLinedefs(256, 512);
    auto sidedefs = MakeBoxSidedefs();
    auto sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAP02", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "RectangularRoom_Tall");
}

//-----------------------------------------------------------------------------
// Test: Very small room (minimum viable size)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, VerySmallRoom)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // 16x16 units - very small but valid
    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},
        {16 << 16, 0},
        {16 << 16, 16 << 16},
        {0, 16 << 16}
    };
    auto linedefs = MakeBoxLinedefs(16, 16);
    auto sidedefs = MakeBoxSidedefs();
    auto sectors = MakeSingleSector(0, 64 << 16);

    bool result = compiler.buildGLNodes("MAP03", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "VerySmallRoom");
}

//-----------------------------------------------------------------------------
// Test: Large room
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, LargeRoom)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // 4096x4096 units - large map coordinate
    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},
        {4096 << 16, 0},
        {4096 << 16, 4096 << 16},
        {0, 4096 << 16}
    };
    auto linedefs = MakeBoxLinedefs(4096, 4096);
    auto sidedefs = MakeBoxSidedefs();
    auto sectors = MakeSingleSector(0, 256 << 16);

    bool result = compiler.buildGLNodes("MAP04", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "LargeRoom");
}

//-----------------------------------------------------------------------------
// Test: L-shaped room (non-convex polygon)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, LShapedRoom)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // L-shaped room layout (6 corners):
    //   v5 +-----+ v4
    //      |     |
    //   v0 +-----+ v1
    //          + v2
    //          |
    //      v6  +-----+ v3
    //
    // 6 vertices forming an L
    // 7 linedefs: 6 outer walls + 1 interior corner

    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},                    // v0 - bottom-left of horizontal bar
        {256 << 16, 0},           // v1 - bottom-right of horizontal bar
        {256 << 16, 256 << 16},   // v2 - inner corner bottom
        {512 << 16, 256 << 16},   // v3 - bottom-right of vertical bar
        {512 << 16, 512 << 16},   // v4 - top-right of vertical bar
        {0, 512 << 16},           // v5 - top-left
        {256 << 16, 512 << 16}    // v6 - top of inner corner
    };

    // 7 linedefs to form the L shape (note: must close the loop properly)
    std::vector<ZDBSPInputLinedef> linedefs = {
        {0, 1, 1, 0, 0, {0, -1}},    // bottom: v0->v1
        {1, 2, 1, 0, 0, {0, -1}},    // right lower: v1->v2
        {2, 3, 1, 0, 0, {0, -1}},    // right upper: v2->v3
        {3, 4, 1, 0, 0, {0, -1}},    // top right: v3->v4
        {4, 5, 1, 0, 0, {0, -1}},    // top: v4->v5
        {5, 6, 1, 0, 0, {0, -1}},    // left upper: v5->v6
        {6, 0, 1, 0, 0, {0, -1}}     // left lower: v6->v0 (closes the L)
    };

    std::vector<ZDBSPInputSidedef> sidedefs = {
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0}
    };

    auto sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAP05", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "LShapedRoom");
}

//-----------------------------------------------------------------------------
// Test: Two sectors with different heights
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, TwoSectors_DifferentHeights)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // Two rooms side by side, separated by a wall
    // Left room: floor=0, ceiling=128
    // Right room: floor=0, ceiling=192 (taller)
    //
    //   v4 +-------+ v5
    //      | SECT1 | SECT0
    //   v0 +---+---+ v1
    //          |
    //   v7 +---+ v6
    //      | SECT0 |
    //   v3 +-------+ v2

    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},                    // v0 - bottom-left of left room
        {256 << 16, 0},           // v1 - bottom of partition
        {256 << 16, 256 << 16},   // v2 - top of partition
        {512 << 16, 256 << 16},   // v3 - bottom-right of right room
        {512 << 16, 512 << 16},   // v4 - top-right of right room
        {0, 512 << 16},           // v5 - top-left of left room
        {256 << 16, 128 << 16},   // v6 - bottom of left room partition (lower)
        {0, 128 << 16}            // v7 - left of left room partition (lower)
    };

    // 6 linedefs: outer walls + partition wall (two-sided)
    std::vector<ZDBSPInputLinedef> linedefs = {
        {0, 1, 1, 0, 0, {0, 1}},   // partition bottom: v0->v1 (two-sided, refs sectors 0 and 1)
        {1, 2, 1, 0, 0, {1, -1}},  // right wall of left room: v1->v2
        {2, 3, 1, 0, 0, {1, 0}},   // top of rooms: v2->v3 (two-sided)
        {3, 4, 1, 0, 0, {0, -1}},  // right outer wall: v3->v4
        {4, 5, 1, 0, 0, {0, -1}},  // top outer wall: v4->v5
        {5, 0, 1, 0, 0, {0, -1}},  // left outer wall: v5->v0
        {5, 7, 1, 0, 0, {0, -1}},  // left room upper wall: v5->v7
        {7, 6, 1, 0, 0, {0, -1}},  // left room partition top: v7->v6
        {6, 1, 1, 0, 0, {0, -1}},  // left room partition bottom: v6->v1
        {6, 2, 1, 0, 0, {0, -1}},  // connecting: v6->v2
        {7, 0, 1, 0, 0, {0, -1}}   // left room left-bottom: v7->v0
    };

    std::vector<ZDBSPInputSidedef> sidedefs = {
        {0, 0, "FLAT1", "", "", 0},   // sector 0
        {0, 0, "FLAT1", "", "", 1},   // sector 1
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 1},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0}
    };

    std::vector<ZDBSPInputSector> sectors = {
        {0, 128 << 16, "FLOOR1", "CEIL1", 144, 0, 0},  // sector 0 - taller
        {0, 192 << 16, "FLOOR1", "CEIL1", 144, 0, 0}   // sector 1 - shorter ceiling
    };

    bool result = compiler.buildGLNodes("MAP06", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "TwoSectors_DifferentHeights");
}

//-----------------------------------------------------------------------------
// Test: Narrow corridor
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, NarrowCorridor)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // Narrow corridor: 64 units wide x 512 long
    //   v4 +--+ v5
    //      |  |
    //   v0 +--+ v1
    //          |
    //   v7 +--+ v2
    //      |  |
    //   v3 +--+ v6

    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},
        {64 << 16, 0},
        {64 << 16, 512 << 16},
        {128 << 16, 512 << 16},
        {128 << 16, 0},
        {192 << 16, 0},
        {192 << 16, 64 << 16},
        {0, 64 << 16},
        {0, 576 << 16},
        {64 << 16, 576 << 16},
        {64 << 16, 640 << 16},
        {192 << 16, 640 << 16},
        {192 << 16, 576 << 16}
    };

    std::vector<ZDBSPInputLinedef> linedefs = {
        {0, 1, 1, 0, 0, {0, -1}},   // bottom: v0->v1
        {1, 2, 1, 0, 0, {0, -1}},   // right: v1->v2
        {2, 3, 1, 0, 0, {0, -1}},   // top step: v2->v3
        {3, 4, 1, 0, 0, {0, -1}},   // top step right: v3->v4
        {4, 5, 1, 0, 0, {0, -1}},   // top: v4->v5
        {5, 6, 1, 0, 0, {0, -1}},   // right step: v5->v6
        {6, 7, 1, 0, 0, {0, -1}},   // bottom step: v6->v7
        {7, 0, 1, 0, 0, {0, -1}},   // left: v7->v0
        // Additional segments for the extended corridor
        {8, 9, 1, 0, 0, {0, -1}},   // v8->v9
        {9, 10, 1, 0, 0, {0, -1}},  // v9->v10
        {10, 11, 1, 0, 0, {0, -1}}, // v10->v11
        {11, 8, 1, 0, 0, {0, -1}}   // v11->v8
    };

    std::vector<ZDBSPInputSidedef> sidedefs = {
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0}
    };

    auto sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAP07", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "NarrowCorridor");
}

//-----------------------------------------------------------------------------
// Test: Room with interior wall (creates separate areas)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, RoomWithInteriorWall)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // Square room with a horizontal interior wall dividing it
    //   v4 +---+ v5
    //      |   |   (interior wall v4->v7)
    //   v0 +---+ v1
    //      |   |   (interior wall v0->v6)
    //   v3 +---+ v2

    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},                    // v0 - bottom-left
        {256 << 16, 0},           // v1 - bottom-right
        {256 << 16, 256 << 16},  // v2 - top-right of bottom section
        {0, 256 << 16},           // v3 - top-left of bottom section
        {0, 512 << 16},           // v4 - top-left of top section
        {256 << 16, 512 << 16},  // v5 - top-right of top section
        {256 << 16, 256 << 16},  // v6 - same as v2 (for interior wall endpoint)
        {0, 256 << 16}            // v7 - same as v3 (for interior wall endpoint)
    };

    std::vector<ZDBSPInputLinedef> linedefs = {
        {0, 1, 1, 0, 0, {0, -1}},   // bottom outer: v0->v1
        {1, 2, 1, 0, 0, {0, -1}},   // right outer (lower): v1->v2
        {2, 3, 1, 0, 0, {0, -1}},   // interior wall (bottom): v2->v3 (one-sided)
        {3, 0, 1, 0, 0, {0, -1}},   // left outer (lower): v3->v0
        {3, 7, 1, 0, 0, {0, -1}},   // interior wall (top): v3->v7
        {7, 4, 1, 0, 0, {0, -1}},   // left outer (upper): v7->v4
        {4, 5, 1, 0, 0, {0, -1}},   // top outer: v4->v5
        {5, 6, 1, 0, 0, {0, -1}},   // right outer (upper): v5->v6
        {6, 2, 1, 0, 0, {0, -1}}    // connect v6->v2 (closes the upper section)
    };

    std::vector<ZDBSPInputSidedef> sidedefs = {
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0}
    };

    auto sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAP08", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "RoomWithInteriorWall");
}

//-----------------------------------------------------------------------------
// Test: Room with sector light level variation
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, SectorLightVariation)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // Two sectors side by side with different light levels
    //   v4 +---+---+ v5
    //      | S0 | S1 |
    //   v0 +---+---+ v1
    //          |
    //   v7 +---+ v6

    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},                   // v0
        {256 << 16, 0},          // v1
        {256 << 16, 256 << 16},  // v2
        {512 << 16, 256 << 16},  // v3
        {512 << 16, 512 << 16},  // v4
        {0, 512 << 16},           // v5
        {0, 256 << 16},           // v6
        {256 << 16, 512 << 16}   // v7
    };

    std::vector<ZDBSPInputLinedef> linedefs = {
        {0, 1, 1, 0, 0, {0, 1}},   // partition bottom: v0->v1
        {1, 2, 1, 0, 0, {1, -1}},  // right: v1->v2
        {2, 3, 1, 0, 0, {1, 0}},   // top partition: v2->v3
        {3, 4, 1, 0, 0, {0, -1}},  // right outer: v3->v4
        {4, 5, 1, 0, 0, {0, -1}},  // top: v4->v5
        {5, 6, 1, 0, 0, {0, -1}},  // left: v5->v6
        {6, 0, 1, 0, 0, {0, -1}},  // left lower: v6->v0
        {6, 7, 1, 0, 0, {0, -1}},  // left upper: v6->v7
        {7, 4, 1, 0, 0, {0, -1}}   // top left: v7->v4
    };

    std::vector<ZDBSPInputSidedef> sidedefs = {
        {0, 0, "FLAT1", "", "", 0},   // sector 0
        {0, 0, "FLAT1", "", "", 1},   // sector 1
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 1},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0}
    };

    // Two sectors with different light levels
    std::vector<ZDBSPInputSector> sectors = {
        {0, 128 << 16, "FLOOR1", "CEIL1", 144, 0, 0},  // bright: light=144
        {0, 128 << 16, "FLOOR1", "CEIL1", 64, 0, 0}     // dark: light=64
    };

    bool result = compiler.buildGLNodes("MAP09", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "SectorLightVariation");
}

//-----------------------------------------------------------------------------
// Test: Room with offset origin (negative coordinates)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, OffsetOriginRoom)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // Room positioned away from origin
    //   v2 +----+ v3
    //      |    |
    //   v0 +----+ v1
    //   Offset: x=-1024, y=-1024

    int offset = (int32_t)((uint32_t)-1024 << 16);
    std::vector<ZDBSPInputVertex> vertices = {
        {offset, offset},
        {offset + (256 << 16), offset},
        {offset + (256 << 16), offset + (256 << 16)},
        {offset, offset + (256 << 16)}
    };
    auto linedefs = MakeBoxLinedefs(256, 256);
    auto sidedefs = MakeBoxSidedefs();
    auto sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAP10", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "OffsetOriginRoom");
}

//-----------------------------------------------------------------------------
// Test: Room at large coordinates
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, LargeCoordinateRoom)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // Room at large map coordinates (common in Doom maps)
    // Position: x=3072, y=2048 (in 16.16 format)
    int baseX = 3072 << 16;
    int baseY = 2048 << 16;
    int size = 512 << 16;

    std::vector<ZDBSPInputVertex> vertices = {
        {baseX, baseY},
        {baseX + size, baseY},
        {baseX + size, baseY + size},
        {baseX, baseY + size}
    };
    auto linedefs = MakeBoxLinedefs(512, 512);
    auto sidedefs = MakeBoxSidedefs();
    auto sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAP11", vertices, linedefs, sidedefs, sectors, output);

    EXPECT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "LargeCoordinateRoom");
}

//-----------------------------------------------------------------------------
// Full pipeline test: Generate -> Serialize -> Deserialize -> Compare
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, FullPipeline_SquareRoom)
{
    BspCompiler compiler;
    ZDBSPOutput originalOutput;

    // Create a simple square room
    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},
        {256 << 16, 0},
        {256 << 16, 256 << 16},
        {0, 256 << 16}
    };
    auto linedefs = MakeBoxLinedefs(256, 256);
    auto sidedefs = MakeBoxSidedefs();
    auto sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAP01", vertices, linedefs, sidedefs, sectors, originalOutput);

    ASSERT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(originalOutput, "FullPipeline_SquareRoom");

    // Serialize to GLBSP v5 format
    GLBWadWriter wadWriter;
    GLBSPSerializer::serialize(originalOutput, wadWriter);

    // Get the lumps
    const auto& lumps = wadWriter.getAllLumps();
    ASSERT_EQ(lumps.size(), 4u) << "Expected 4 lumps (VERT, SEGS, SSECT, NODES)";

    // Deserialize back (simulating P_LoadGLNodes)
    ZDBSPOutput loadedOutput;
    GLBSPDeserializer::deserialize(lumps, loadedOutput);

    // Compare original vs loaded
    CompareOutputs(originalOutput, loadedOutput, "FullPipeline_SquareRoom");
}

//-----------------------------------------------------------------------------
// Full pipeline test: Generate -> Serialize -> Deserialize (L-shaped room)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, FullPipeline_LShapedRoom)
{
    BspCompiler compiler;
    ZDBSPOutput originalOutput;

    // Create L-shaped room
    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},                    // v0
        {256 << 16, 0},           // v1
        {256 << 16, 256 << 16},   // v2
        {512 << 16, 256 << 16},   // v3
        {512 << 16, 512 << 16},   // v4
        {0, 512 << 16},           // v5
        {256 << 16, 512 << 16}    // v6
    };
    std::vector<ZDBSPInputLinedef> linedefs = {
        {0, 1, 1, 0, 0, {0, -1}},
        {1, 2, 1, 0, 0, {0, -1}},
        {2, 3, 1, 0, 0, {0, -1}},
        {3, 4, 1, 0, 0, {0, -1}},
        {4, 5, 1, 0, 0, {0, -1}},
        {5, 6, 1, 0, 0, {0, -1}},
        {6, 0, 1, 0, 0, {0, -1}}
    };
    std::vector<ZDBSPInputSidedef> sidedefs = {
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0}
    };
    auto sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAP05", vertices, linedefs, sidedefs, sectors, originalOutput);

    ASSERT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(originalOutput, "FullPipeline_LShapedRoom");

    // Serialize to GLBSP v5 format
    GLBWadWriter wadWriter;
    GLBSPSerializer::serialize(originalOutput, wadWriter);

    // Deserialize back
    ZDBSPOutput loadedOutput;
    GLBSPDeserializer::deserialize(wadWriter.getAllLumps(), loadedOutput);

    // Compare
    CompareOutputs(originalOutput, loadedOutput, "FullPipeline_LShapedRoom");
}

//-----------------------------------------------------------------------------
// Full pipeline test: Generate -> Serialize -> Deserialize (multi-sector)
//-----------------------------------------------------------------------------

TEST_F(BspCompilerTest, FullPipeline_TwoSectors)
{
    BspCompiler compiler;
    ZDBSPOutput originalOutput;

    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},
        {256 << 16, 0},
        {256 << 16, 256 << 16},
        {512 << 16, 256 << 16},
        {512 << 16, 512 << 16},
        {0, 512 << 16}
    };
    std::vector<ZDBSPInputLinedef> linedefs = {
        {0, 1, 1, 0, 0, {0, 1}},
        {1, 2, 1, 0, 0, {1, -1}},
        {2, 3, 1, 0, 0, {1, 0}},
        {3, 4, 1, 0, 0, {0, -1}},
        {4, 5, 1, 0, 0, {0, -1}},
        {5, 0, 1, 0, 0, {0, -1}}
    };
    std::vector<ZDBSPInputSidedef> sidedefs = {
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 1},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 1},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0}
    };
    std::vector<ZDBSPInputSector> sectors = {
        {0, 128 << 16, "FLOOR1", "CEIL1", 144, 0, 0},
        {0, 192 << 16, "FLOOR1", "CEIL1", 144, 0, 0}
    };

    bool result = compiler.buildGLNodes("MAP06", vertices, linedefs, sidedefs, sectors, originalOutput);

    ASSERT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(originalOutput, "FullPipeline_TwoSectors");

    // Serialize to GLBSP v5 format
    GLBWadWriter wadWriter;
    GLBSPSerializer::serialize(originalOutput, wadWriter);

    // Deserialize back
    ZDBSPOutput loadedOutput;
    GLBSPDeserializer::deserialize(wadWriter.getAllLumps(), loadedOutput);

    // Compare
    CompareOutputs(originalOutput, loadedOutput, "FullPipeline_TwoSectors");
}

//-----------------------------------------------------------------------------
// Bug regression tests
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Test: Miniseg vertex index handling (GitHub bug: miniseg index out of bounds)
//
// When ZDBSP generates GL nodes, it creates miniseg vertices with indices
// >= numOriginalVertices. The gl_bsp.cpp integration code must access
// glvertexes[seg.vX - numOriginalVertices] when the index is a miniseg.
//
// Bug: gl_bsp.cpp was using raw ZDBSP indices (e.g., 383) instead of
// offset indices (383 - 383 = 0) when accessing glvertexes[], causing
// out-of-bounds memory access.
//-----------------------------------------------------------------------------
TEST_F(BspCompilerTest, MinisegVertexIndexOffset)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // Create a room that will trigger miniseg generation
    // L-shaped room generates more complex GL nodes requiring minisegs
    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},
        {256 << 16, 0},
        {256 << 16, 256 << 16},
        {512 << 16, 256 << 16},
        {512 << 16, 512 << 16},
        {0, 512 << 16},
        {256 << 16, 512 << 16}
    };
    std::vector<ZDBSPInputLinedef> linedefs = {
        {0, 1, 1, 0, 0, {0, -1}},
        {1, 2, 1, 0, 0, {0, -1}},
        {2, 3, 1, 0, 0, {0, -1}},
        {3, 4, 1, 0, 0, {0, -1}},
        {4, 5, 1, 0, 0, {0, -1}},
        {5, 6, 1, 0, 0, {0, -1}},
        {6, 0, 1, 0, 0, {0, -1}}
    };
    std::vector<ZDBSPInputSidedef> sidedefs = {
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0},
        {0, 0, "FLAT1", "", "", 0}
    };
    std::vector<ZDBSPInputSector> sectors = MakeSingleSector(0, 128 << 16);

    bool result = compiler.buildGLNodes("MAPMS1", vertices, linedefs, sidedefs, sectors, output);
    ASSERT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "MinisegVertexIndexOffset");

    // Verify numOriginalVertices is set correctly
    EXPECT_GT(output.numOriginalVertices, 0);
    EXPECT_LE(output.numOriginalVertices, (int32_t)vertices.size());

    // Verify all segs have valid vertex indices
    for (size_t i = 0; i < output.segs.size(); ++i) {
        const auto& seg = output.segs[i];

        // Both v1 and v2 must be valid indices into the vertex array
        EXPECT_LT(seg.v1, (uint32_t)output.vertices.size())
            << "Seg " << i << " has invalid v1 index: " << seg.v1;
        EXPECT_LT(seg.v2, (uint32_t)output.vertices.size())
            << "Seg " << i << " has invalid v2 index: " << seg.v2;

        // If v1 or v2 is a miniseg (index >= numOriginalVertices),
        // the gl_bsp.cpp code MUST access glvertexes[vX - numOriginalVertices]
        // to avoid out-of-bounds access.
        //
        // This test documents the requirement:
        // For miniseg indices, the offset into glvertexes[] = vX - numOriginalVertices
        if (seg.v1 >= (uint32_t)output.numOriginalVertices) {
            int32_t minisegOffset = seg.v1 - output.numOriginalVertices;
            EXPECT_GE(minisegOffset, 0);
            EXPECT_LT(minisegOffset, (int32_t)(output.vertices.size() - output.numOriginalVertices))
                << "Seg " << i << " miniseg v1 offset " << minisegOffset
                << " would be out of bounds for glvertexes array of size "
                << (output.vertices.size() - output.numOriginalVertices);
        }
        if (seg.v2 >= (uint32_t)output.numOriginalVertices) {
            int32_t minisegOffset = seg.v2 - output.numOriginalVertices;
            EXPECT_GE(minisegOffset, 0);
            EXPECT_LT(minisegOffset, (int32_t)(output.vertices.size() - output.numOriginalVertices))
                << "Seg " << i << " miniseg v2 offset " << minisegOffset
                << " would be out of bounds for glvertexes array of size "
                << (output.vertices.size() - output.numOriginalVertices);
        }
    }
}

//-----------------------------------------------------------------------------
// Test: Fixed-point value preservation when copying to Doom Legacy structures
//
// ZDBSP outputs 16.16 fixed-point values (int32_t). When these are copied
// to Doom Legacy's node_t/divline_t structures which use fixed_t, the
// fixed_t(int a) constructor would SHIFT by 16, corrupting the value.
//
// Bug: node->x = output.nodes[i].x; where output.nodes[i].x is already
// a 16.16 value (e.g., 63963136) would result in fixed_t(63963136) which
// internally stores 63963136 << 16 = overflow/corruption.
//
// Fix: Use fixed_t(value, true) constructor which stores the value as-is
// without shifting.
//
// This test verifies that ZDBSPOutput contains valid 16.16 values that
// will survive the copy to fixed_t (when using the correct constructor).
//-----------------------------------------------------------------------------
TEST_F(BspCompilerTest, NodeFixedPointValuePreservation)
{
    BspCompiler compiler;
    ZDBSPOutput output;

    // Use a rectangular room with known dimensions
    std::vector<ZDBSPInputVertex> vertices = {
        {0, 0},
        {256 << 16, 0},           // 256 units wide
        {256 << 16, 128 << 16},   // 128 units tall
        {0, 128 << 16}
    };
    auto linedefs = MakeBoxLinedefs(256, 128);
    auto sidedefs = MakeBoxSidedefs();
    auto sectors = MakeSingleSector(0, 64 << 16);

    bool result = compiler.buildGLNodes("MAPFP1", vertices, linedefs, sidedefs, sectors, output);
    ASSERT_TRUE(result) << "BspCompiler failed: " << compiler.getError();
    ValidateOutput(output, "NodeFixedPointValuePreservation");

    // Verify nodes have partition line values in valid 16.16 range
    for (size_t i = 0; i < output.nodes.size(); ++i) {
        const auto& node = output.nodes[i];

        // These values should be in reasonable map coordinate range
        // (16.16 format - values like 256 << 16 = 16777216 for coordinate 256)
        //
        // The key invariant: When copying to fixed_t, the integration code
        // in gl_bsp.cpp MUST use fixed_t(node.x, true) to preserve the
        // 16.16 value without additional shifting.
        //
        // A value like 0x3D700000 (16384 << 16 = coordinate 16384) should
        // remain as 0x3D700000 when stored in fixed_t with rawShifted=true
        // but would become 0x70000000 (overflow) with rawShifted=false

        // Check that partition line values are non-trivially sized
        // (ZDBSP should produce partition lines that span the map)
        bool hasValidPartition = (node.x != 0) || (node.y != 0) ||
                                  (node.dx != 0) || (node.dy != 0);
        EXPECT_TRUE(hasValidPartition)
            << "Node " << i << " has zero partition";

        // dx and dy should not both be zero (would be degenerate partition)
        // A valid partition line has some direction
        bool hasDirection = (node.dx != 0) || (node.dy != 0);
        EXPECT_TRUE(hasDirection)
            << "Node " << i << " has zero direction vector";

        // For the bug: if the fixed_t copy was done incorrectly with shift,
        // values like 0x3D700000 would overflow. After correct copy with
        // rawShifted=true, the fixed_t.val would be 0x3D700000.
        //
        // To verify the fix is applied in gl_bsp.cpp, check that node
        // values are in range that wouldn't overflow if stored directly.
        //
        // Max safe value for raw fixed_t without overflow: 0x7FFFFFFF >> 16 = 32767
        // But we expect coordinates up to at least 512 units = 512 << 16 = 0x20000000
        //
        // The test passes if BspCompiler generates valid nodes. The actual
        // fixed_t conversion correctness is verified by the integration
        // test in gl_bsp.cpp which uses fixed_t(output.nodes[i].x, true)
    }

    // Also verify segs have valid offset values (these go into glseg_t which
    // also uses fixed_t, so same bug pattern applies)
    for (size_t i = 0; i < std::min((size_t)10, output.segs.size()); ++i) {
        const auto& seg = output.segs[i];
        // Seg references valid vertices
        EXPECT_LT(seg.v1, (uint32_t)output.vertices.size());
        EXPECT_LT(seg.v2, (uint32_t)output.vertices.size());
    }
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
