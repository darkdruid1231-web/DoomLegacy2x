//-----------------------------------------------------------------------------
//
// ZDBSP Integration for Doom Legacy
//
//-----------------------------------------------------------------------------

#include "zdbsp_integration.h"
#include "doomdef.h"
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#endif

namespace ZDBSPIntegration
{

// Get current working directory
static std::string GetCurrentDir()
{
    char buffer[1024];
#ifdef _WIN32
    GetCurrentDirectoryA(sizeof(buffer), buffer);
#else
    getcwd(buffer, sizeof(buffer));
#endif
    return std::string(buffer);
}

// Find WAD file in current directory
static std::string FindWadInDir()
{
    // Common WAD filenames to check
    const char* wadFiles[] = {
        "DOOM2.WAD",
        "DOOM.WAD",
        "DOOM_OG.WAD",
        "NERVE.WAD",
        "TNT.WAD",
        "PLUTONIA.WAD",
        "FREEDOOM.WAD",
        "freedoom2.wad",
        "freedoom.wad",
    };

    std::string currentDir = GetCurrentDir();

    for (const char* wadFile : wadFiles)
    {
        std::string fullPath = currentDir + "\\" + wadFile;
        struct stat buffer;
        if (stat(fullPath.c_str(), &buffer) == 0)
        {
            return fullPath;
        }
    }

    return "";
}

std::string GetZDBSPPath()
{
    // Check various possible locations for zdbsp
    static std::string zdbspPath;

    if (!zdbspPath.empty())
        return zdbspPath;

#ifdef _WIN32
    const char* possiblePaths[] = {
        ".\\zdbsp.exe",                       // Same directory as doomlegacy.exe
        "zdbsp.exe",                          // Current directory
    };
#else
    const char* possiblePaths[] = {
        "./zdbsp",
        "zdbsp",
    };
#endif

    for (const auto& path : possiblePaths)
    {
        struct stat buffer;
        if (stat(path, &buffer) == 0)
        {
            zdbspPath = path;
            return zdbspPath;
        }
    }

    return "";
}

bool IsZDBSPAvailable()
{
    return !GetZDBSPPath().empty();
}

bool GenerateGLNodes(const std::string& mapName)
{
    std::string zdbspPath = GetZDBSPPath();
    if (zdbspPath.empty())
    {
        CONS_Printf("zdbsp not found - cannot generate GL nodes\n");
        return false;
    }

    // Find the WAD file
    std::string wadPath = FindWadInDir();
    if (wadPath.empty())
    {
        CONS_Printf("Could not find WAD file\n");
        return false;
    }

    CONS_Printf("Using zdbsp to generate GL nodes from %s...\n", wadPath.c_str());

    // Build command line: zdbsp -g -x -m MAP01 -o output.wad input.wad
    // -g: build GL-friendly nodes
    // -x: GL-only (don't rebuild normal nodes)
    // -m: map name
    // -o: output file (same as input to overwrite)

    std::string cmd = "\"" + zdbspPath + "\" -g -x -m " + mapName + " -o \"" + wadPath + "\" \"" + wadPath + "\"";

    CONS_Printf("Running: %s\n", cmd.c_str());

    // Run zdbsp
    int result = system(cmd.c_str());

    if (result != 0)
    {
        CONS_Printf("zdbsp failed with code %d\n", result);
        return false;
    }

    CONS_Printf("zdbsp generated GL nodes successfully\n");
    return true;
}

// Generate GL nodes for all maps in WAD if needed
void CheckAndGenerateGLNodes()
{
    if (!IsZDBSPAvailable())
        return;

    std::string wadPath = FindWadInDir();
    if (wadPath.empty())
        return;

    CONS_Printf("WAD missing GL nodes, generating with zdbsp...\n");

    // Run zdbsp without -m flag to process all maps
    std::string zdbspPath = GetZDBSPPath();
    std::string cmd = "\"" + zdbspPath + "\" -g -x -o \"" + wadPath + "\" \"" + wadPath + "\"";

    CONS_Printf("Running: %s\n", cmd.c_str());

    int result = system(cmd.c_str());

    if (result == 0)
    {
        CONS_Printf("zdbsp generated GL nodes for all maps\n");
    }
    else
    {
        CONS_Printf("zdbsp failed with code %d\n", result);
    }
}

} // namespace ZDBSPIntegration
