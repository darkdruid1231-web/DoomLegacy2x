// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: gl_bsp.cpp $
//
// Copyright (C) 1998-2007 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Dynamic GL nodes generation for OpenGL rendering.

#include "doomdef.h"
#include "doomdata.h"
#include "g_map.h"
#include "r_defs.h"
#include "z_zone.h"

void BuildGLData(Map *mp)
{
    // Build GL vertexes from regular vertexes
    mp->glvertexes = (mapglvert_t *)Z_Malloc(mp->numvertexes * sizeof(mapglvert_t), PU_LEVEL, 0);
    for (int i = 0; i < mp->numvertexes; i++)
    {
        mp->glvertexes[i].x = mp->vertexes[i].x.Float();
        mp->glvertexes[i].y = mp->vertexes[i].y.Float();
    }

    // Build GL segs from regular segs
    mp->glsegs = (mapglseg_t *)Z_Malloc(mp->numsegs * sizeof(mapglseg_t), PU_LEVEL, 0);
    for (int i = 0; i < mp->numsegs; i++)
    {
        seg_t *seg = &mp->segs[i];
        mapglseg_t *glseg = &mp->glsegs[i];
        glseg->v1 = seg->v1 - mp->vertexes;
        glseg->v2 = seg->v2 - mp->vertexes;
        glseg->linedef = seg->linedef - mp->lines;
        glseg->side = seg->side;
        glseg->partner = seg->partner_seg ? (seg->partner_seg - mp->segs) : 0xFFFF;
    }

    // Build GL subsectors from regular subsectors
    mp->glsubsectors = (mapglsubsector_t *)Z_Malloc(mp->numsubsectors * sizeof(mapglsubsector_t), PU_LEVEL, 0);
    for (int i = 0; i < mp->numsubsectors; i++)
    {
        subsector_t *sub = &mp->subsectors[i];
        mapglsubsector_t *glsub = &mp->glsubsectors[i];
        glsub->first_seg = sub->first_seg;
        glsub->num_segs = sub->num_segs;
    }

    // For PVS, create a simple all-visible vis data
    int vissize = ((mp->numsubsectors + 7) / 8) * mp->numsubsectors;
    mp->glvis = (byte *)Z_Malloc(vissize, PU_LEVEL, 0);
    memset(mp->glvis, 0xFF, vissize); // all subsectors visible from all
}