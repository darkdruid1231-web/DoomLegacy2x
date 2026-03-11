//-----------------------------------------------------------------------------
//
// Deferred rendering G-Buffer implementation for Doom Legacy
//
// This is a stub implementation - full deferred rendering requires OpenGL 3.0+
//-----------------------------------------------------------------------------

// Include GLEW first to avoid conflicts
#include <GL/glew.h>

#include "hw_gbuffer.h"
#include "oglrenderer.hpp"
#include "oglshaders.h"
#include "doomdef.h"
#include "command.h"

// Global instance
DeferredRenderer* g_deferredRenderer = nullptr;

// DeferredRenderer implementation
DeferredRenderer::DeferredRenderer()
    : useDeferred(false), maxLightsPerSurface(32),
      bloomEnabled(true), bloomIntensity(0.5f), fogDensity(0.001f)
{
    fogColor[0] = fogColor[1] = fogColor[2] = 0.0f;
}

DeferredRenderer::~DeferredRenderer()
{
    Destroy();
}

bool DeferredRenderer::Init(int width, int height)
{
    // Deferred rendering requires OpenGL 3.0+
    // For now, just initialize the settings
    useDeferred = false;  // Always use forward rendering for compatibility
    bloomEnabled = true;
    bloomIntensity = 0.5f;
    fogDensity = 0.001f;
    maxLightsPerSurface = 32;

    CONS_Printf("Deferred renderer initialized (forward rendering mode)\n");
    return true;
}

void DeferredRenderer::BeginGeometryPass()
{
    // Stub - in deferred mode, this would bind G-Buffer
}

void DeferredRenderer::EndGeometryPass()
{
    // Stub
}

void DeferredRenderer::BeginLightingPass()
{
    // Stub - in deferred mode, this would enable additive blending
}

void DeferredRenderer::RenderLight(const DynamicLight& light)
{
    // Stub - in deferred mode, this would render light volumes
}

void DeferredRenderer::EndLightingPass()
{
    // Stub
}

void DeferredRenderer::BeginPostProcess()
{
    // Stub
}

void DeferredRenderer::EndPostProcess()
{
    // Stub
}

void DeferredRenderer::ApplyBloom()
{
    // Stub - bloom not available in forward rendering mode
}

void DeferredRenderer::ApplyFog(float viewZ)
{
    // Stub - fog handled by standard OpenGL
}

void DeferredRenderer::Destroy()
{
    // Stub - nothing to clean up in forward rendering mode
    CONS_Printf("Deferred renderer destroyed\n");
}
