// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2024 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Data-driven light emitter definitions loaded from LIGHTDEFS lumps.

#ifndef hw_lightdefs_h
#define hw_lightdefs_h 1

#include "info.h"    // mobjtype_t

struct FrameLight;
class DActor;

/// Light emitter definition (read from LIGHTDEFS lump or the hardcoded table).
struct LightEmitterDef
{
    mobjtype_t type;    ///< Actor type that emits this light.
    float radius;       ///< Attenuation radius in world units.
    float r, g, b;      ///< Light color (0-1).
    float flicker;      ///< Flicker amplitude fraction (0 = steady).
};

/// Parse a LIGHTDEFS text lump and merge entries into the runtime table.
/// Safe to call multiple times (later entries overwrite earlier ones for the
/// same actor type).  lump is a WAD lump number; pass -1 to skip.
void ParseLightDefs(int lump);

/// Fill `out` for the actor `da` if it has a registered light definition.
/// Returns true if a definition was found and out was filled.
bool GetActorLight(const DActor *da, FrameLight &out);

/// Called once at startup to populate the table from the hardcoded defaults.
void InitLightDefs();

#endif // hw_lightdefs_h
