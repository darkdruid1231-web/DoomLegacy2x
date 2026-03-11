//-----------------------------------------------------------------------------
//
// Deferred rendering G-Buffer for Doom Legacy
//
// G-Buffer contains:
// - Position buffer (RGBA32F: xyz + depth)
// - Normal buffer (RGBA16F: normal xyz + material ID)
// - Albedo buffer (RGBA: diffuse color)
// - Depth buffer (from default depth)
//
//-----------------------------------------------------------------------------

#ifndef hw_gbuffer_h
#define hw_gbuffer_h

#include "doomdef.h"

// Forward declarations
class OGLRenderer;

// Light types (matching GZDoom)
enum LightType {
    LIGHT_POINT = 0,
    LIGHT_PULSE,
    LIGHT_FLICKER,
    LIGHT_RANDOM,
    LIGHT_SECTOR,
    LIGHT_COLORPULSE,
    LIGHT_COLORFLICKER,
    LIGHT_SPOT
};

// Light flags (matching GZDoom)
enum LightFlags {
    LF_SUBTRACTIVE   = 0x01,
    LF_ADDITIVE      = 0x02,
    LF_DONTLIGHTSELF = 0x04,
    LF_ATTENUATE     = 0x08,
    LF_NOSHADOWMAP   = 0x10,
    LF_DONTLIGHTACTORS = 0x20,
    LF_SPOT          = 0x40,
    LF_DONTLIGHTOTHERS = 0x80,
    LF_DONTLIGHTMAP  = 0x100
};

// Dynamic light structure
struct DynamicLight {
    float x, y, z;          // Position
    float color[3];          // RGB color
    float radius;            // Light radius
    float intensity;         // Brightness
    LightType type;          // Light type
    uint32_t flags;         // Light flags

    DynamicLight() : x(0), y(0), z(0), radius(256.0f), intensity(1.0f),
                    type(LIGHT_POINT), flags(LF_ADDITIVE | LF_ATTENUATE) {
        color[0] = color[1] = color[2] = 1.0f;
    }
};

// Light list for a surface
struct SurfaceLightList {
    int numLights;
    DynamicLight** lights;
    float* distances;

    SurfaceLightList() : numLights(0), lights(nullptr), distances(nullptr) {}
};

// Deferred renderer class - stub implementation for future use
class DeferredRenderer {
private:
    bool useDeferred;
    int maxLightsPerSurface;

    // Feature flags
    bool bloomEnabled;
    float bloomIntensity;
    float fogDensity;
    float fogColor[3];

public:
    DeferredRenderer();
    ~DeferredRenderer();

    // Initialize deferred renderer
    bool Init(int width, int height);

    // Set rendering mode
    void SetDeferredMode(bool deferred) { useDeferred = deferred; }
    bool IsDeferredMode() const { return useDeferred; }

    // Geometry pass - render scene to G-Buffer (stub)
    void BeginGeometryPass();
    void EndGeometryPass();

    // Lighting pass - apply lights (stub)
    void BeginLightingPass();
    void RenderLight(const DynamicLight& light);
    void EndLightingPass();

    // Post-processing (stub)
    void ApplyBloom();
    void ApplyFog(float viewZ);
    void BeginPostProcess();
    void EndPostProcess();

    // Toggle features
    void EnableBloom(bool enable) { bloomEnabled = enable; }
    void SetBloomIntensity(float intensity) { bloomIntensity = intensity; }
    void SetFogDensity(float density) { fogDensity = density; }
    void SetFogColor(float r, float g, float b) { fogColor[0] = r; fogColor[1] = g; fogColor[2] = b; }

    // Cleanup
    void Destroy();

    // Check if deferred rendering is available
    bool IsAvailable() const { return false; }  // Currently disabled
};

// Global instance
extern DeferredRenderer* g_deferredRenderer;

#endif // hw_gbuffer_h
