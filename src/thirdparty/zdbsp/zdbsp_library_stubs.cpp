// Library stubs for ZDBSP - provides globals and utility functions
// that are defined in main.cpp for the CLI tool but needed by the library

#include "zdbsp.h"

#include <cstdio>
#include <cstdarg>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// GLOBAL VARIABLES (normally defined in main.cpp for CLI tool)
// These are declared as 'extern' in zdbsp.h

const char *Map = nullptr;
const char *InName = nullptr;
const char *OutName = nullptr;
bool BuildNodes = true;
bool BuildGLNodes = false;
bool ConformNodes = false;
bool GLOnly = false;
bool WriteComments = false;
bool NoPrune = false;
EBlockmapMode BlockmapMode = EBM_Rebuild;
ERejectMode RejectMode = ERM_DontTouch;
int MaxSegs = 64;
int SplitCost = 8;
int AAPreference = 16;
bool CheckPolyobjs = true;
bool ShowMap = false;
bool CompressNodes = false;
bool CompressGLNodes = false;
bool ForceCompression = false;
bool V5GLNodes = false;
bool HaveSSE1 = false;
bool HaveSSE2 = false;
int SSELevel = 0;

// UTILITY FUNCTIONS (normally defined in main.cpp)

angle_t PointToAngle(fixed_t x, fixed_t y)
{
    double ang = atan2(double(y), double(x));
    const double rad2bam = double(1 << 30) / M_PI;
    double dbam = ang * rad2bam;
    return angle_t(int(dbam)) << 1;
}

static bool ShowWarnings = false;

void Warn(const char *format, ...)
{
    va_list marker;

    if (!ShowWarnings)
    {
        return;
    }

    va_start(marker, format);
    vprintf(format, marker);
    va_end(marker);
}
