//-----------------------------------------------------------------------------
//
// ZDBSP Integration for Doom Legacy
//
// This module provides integration with zdbsp for generating GL nodes
// when they don't exist in the WAD file.
//
// External tool approach: runs zdbsp.exe as a subprocess to generate
// GL nodes, then loads them.
//
//-----------------------------------------------------------------------------

#ifndef ZDBSP_INTEGRATION_H
#define ZDBSP_INTEGRATION_H

#include <string>

namespace ZDBSPIntegration
{
    // Check if zdbsp executable is available
    bool IsZDBSPAvailable();

    // Run zdbsp to generate GL nodes for map in WAD file
    // Returns true if successful, false otherwise
    // If successful, the WAD file will have GL nodes added
    bool GenerateGLNodes(const std::string& mapName);

    // Check WAD for GL nodes and generate them if missing
    // Should be called at startup when using OpenGL mode
    void CheckAndGenerateGLNodes();

    // Get the path to zdbsp executable
    std::string GetZDBSPPath();
}

#endif // ZDBSP_INTEGRATION_H
