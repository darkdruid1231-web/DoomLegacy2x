//-----------------------------------------------------------------------------
//
// BSP Node Compiler - Clean public interface
//
// Uses ZDBSP library for in-process GL node generation.
// ZDBSP headers are isolated in bsp_compiler.cpp only.
//
// All data is passed using plain primitive types (int32_t, uint32_t, etc.)
// to avoid any type conflicts between ZDBSP and Doom Legacy.
//
//-----------------------------------------------------------------------------

#ifndef BSP_COMPILER_H
#define BSP_COMPILER_H

#include <cstdint>
#include <string>
#include <vector>

// Forward declaration
struct BspCompilerImpl;

/// \brief Input vertex for ZDBSP
/// Uses plain int32_t - caller converts from Doom Legacy's fixed_t
struct ZDBSPInputVertex
{
    int32_t x, y;
};

/// \brief Input linedef for ZDBSP
struct ZDBSPInputLinedef
{
    uint32_t v1, v2;           // vertex indices
    uint32_t flags;
    uint32_t special;
    uint32_t tag;
    int32_t sidenum[2];       // side indices (-1 if none)
};

/// \brief Input sidedef for ZDBSP
struct ZDBSPInputSidedef
{
    int32_t textureoffset;    // fixed_t value >> 16
    int32_t rowoffset;
    char toptexture[8];
    char bottomtexture[8];
    char midtexture[8];
    int32_t sector;           // sector index
};

/// \brief Input sector for ZDBSP
struct ZDBSPInputSector
{
    int32_t floorheight;
    int32_t ceilingheight;
    char floorpic[8];
    char ceilingpic[8];
    int32_t lightlevel;
    int32_t special;
    int32_t tag;
};

/// \brief Output node from ZDBSP
struct ZDBSPOutputNode
{
    int32_t x, y, dx, dy;
    int16_t bbox[2][4];
    uint32_t children[2];
};

/// \brief Output subsector from ZDBSP
struct ZDBSPOutputSubsector
{
    uint32_t numlines;
    uint32_t firstline;
};

/// \brief Output GL seg from ZDBSP
struct ZDBSPOutputGLSeg
{
    uint32_t v1, v2;          // vertex indices
    uint32_t linedef;         // linedef index
    uint16_t side;
    uint32_t partner;         // partner seg index
};

/// \brief Output vertex from ZDBSP (includes minisegs)
struct ZDBSPOutputVertex
{
    int32_t x, y;
    int32_t index;
};

/// \brief Complete GL nodes output
struct ZDBSPOutput
{
    std::vector<ZDBSPOutputNode> nodes;
    std::vector<ZDBSPOutputSubsector> subsectors;
    std::vector<ZDBSPOutputGLSeg> segs;
    std::vector<ZDBSPOutputVertex> vertices;
    int32_t numOriginalVertices;  // original map vertices before minisegs
};

/// \brief BSP Node Compiler using ZDBSP library
///
/// This class provides a clean interface to ZDBSP's FNodeBuilder.
/// All ZDBSP headers are isolated in the implementation file.
/// Data is passed using plain primitive types only.
class BspCompiler
{
public:
    BspCompiler();
    ~BspCompiler();

    /// \brief Build GL nodes for a map
    /// \param mapName Name of the map (e.g., "MAP01")
    /// \param vertices Array of vertex data
    /// \param linedefs Array of linedef data
    /// \param sidedefs Array of sidedef data
    /// \param sectors Array of sector data
    /// \param output Receives the compiled GL nodes
    /// \return true if successful
    bool buildGLNodes(const char* mapName,
                      const std::vector<ZDBSPInputVertex>& vertices,
                      const std::vector<ZDBSPInputLinedef>& linedefs,
                      const std::vector<ZDBSPInputSidedef>& sidedefs,
                      const std::vector<ZDBSPInputSector>& sectors,
                      ZDBSPOutput& output);

    /// \brief Check if ZDBSP library is available
    static bool isAvailable();

    /// \brief Get last error message
    const char* getError() const;

private:
    BspCompilerImpl* m_impl;
};

#endif // BSP_COMPILER_H
