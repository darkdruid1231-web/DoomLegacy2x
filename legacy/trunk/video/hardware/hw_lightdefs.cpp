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
/// \brief Data-driven light emitter table (LIGHTDEFS lump support).

#include "hardware/hw_lightdefs.h"
#include "hardware/oglrenderer.hpp"  // FrameLight
#include "g_actor.h"                 // DActor
#include "w_wad.h"
#include "doomdef.h"
#include "console.h"
#include "z_zone.h"
#include <map>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// ---------------------------------------------------------------------------
// Runtime-extensible emitter table keyed by mobjtype_t.
// ---------------------------------------------------------------------------
static std::map<mobjtype_t, LightEmitterDef> g_lightEmitters;

/// Hardcoded defaults — same values as the old static table in oglrenderer.cpp.
static const LightEmitterDef s_defaultDefs[] = {
    { MT_MISC29,       256.0f, 1.0f, 1.0f, 0.9f, 0.0f },
    { MT_MISC30,       192.0f, 1.0f, 1.0f, 0.9f, 0.0f },
    { MT_ARTITORCH,    200.0f, 1.0f, 0.6f, 0.1f, 0.0f },
    { MT_ZTWINEDTORCH, 256.0f, 1.0f, 0.5f, 0.1f, 0.0f },
    { MT_ZWALLTORCH,   192.0f, 1.0f, 0.5f, 0.1f, 0.0f },
    { MT_ZFIREBULL,    256.0f, 1.0f, 0.4f, 0.0f, 0.0f },
    { MT_BRASSTORCH,   192.0f, 1.0f, 0.5f, 0.1f, 0.0f },
    { MT_ZBLUE_CANDLE, 128.0f, 0.4f, 0.5f, 1.0f, 0.0f },
};
static const int NUM_DEFAULT_DEFS =
    (int)(sizeof(s_defaultDefs) / sizeof(s_defaultDefs[0]));

void InitLightDefs()
{
    g_lightEmitters.clear();
    for (int i = 0; i < NUM_DEFAULT_DEFS; i++)
        g_lightEmitters[s_defaultDefs[i].type] = s_defaultDefs[i];

    // Scan all loaded WAD/PK3 files for LIGHTDEFS lumps and parse them.
    int nfiles = (int)fc.Size();
    for (int fi = 0; fi < nfiles; fi++)
    {
        int lump = fc.FindNumForNameFile("LIGHTDEFS", fi);
        if (lump >= 0)
            ParseLightDefs(lump);
    }
}

// ---------------------------------------------------------------------------
// LIGHTDEFS lump parser
// Syntax (line-oriented, case-insensitive):
//
//   pointlight <name> { color <r> <g> <b>; radius <n>; [flicker <f>;] }
//   actor <actorname_or_MT_xxx> light <lightname>
//
// Lines starting with '//' are comments.
// ---------------------------------------------------------------------------

/// Look up mobjtype_t by canonical name (e.g. "MT_MISC29").
/// Returns MT_UNKNOWN (or -1) if not found.
static mobjtype_t FindMobjType(const char *name)
{
    // The mobjinfo[] table stores names as mtypes[] in some ports.
    // For now, do a simple linear scan of the known type names.
    // We support the "MT_xxx" convention.
    for (int t = 0; t < NUMMOBJTYPES; t++)
    {
        const char *tname = mobjinfo[t].classname;
        if (tname && strcasecmp(tname, name) == 0)
            return (mobjtype_t)t;
    }
    return (mobjtype_t)-1;
}

/// Skip whitespace. Returns pointer past whitespace.
static const char *SkipWS(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

/// Skip to end of line.
static const char *SkipLine(const char *p)
{
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;
    return p;
}

void ParseLightDefs(int lump)
{
    if (lump < 0) return;

    int len = fc.LumpLength(lump);
    char *buf = (char *)Z_Malloc(len + 1, PU_DAVE, NULL);
    fc.ReadLump(lump, buf);
    buf[len] = '\0';

    // Named light definitions.
    struct NamedLight {
        char name[64];
        LightEmitterDef def;
    };
    std::map<std::string, LightEmitterDef> namedLights;

    const char *p = buf;
    int linenum = 1;
    while (*p)
    {
        p = SkipWS(p);
        if (!*p) break;

        // Comment
        if (p[0] == '/' && p[1] == '/')
        {
            p = SkipLine(p);
            linenum++;
            continue;
        }

        char token[64];
        if (sscanf(p, "%63s", token) != 1) { p = SkipLine(p); linenum++; continue; }
        p += strlen(token);

        if (strcasecmp(token, "pointlight") == 0)
        {
            char lname[64];
            if (sscanf(p, " %63s", lname) != 1) { p = SkipLine(p); linenum++; continue; }
            p = SkipWS(p + strlen(lname) + 1);

            // Expect opening brace
            if (*p == '{') p++;

            LightEmitterDef def;
            def.type    = (mobjtype_t)-1;
            def.radius  = 128.0f;
            def.r = def.g = def.b = 1.0f;
            def.flicker = 0.0f;

            // Parse key-value pairs until '}'
            while (*p && *p != '}')
            {
                p = SkipWS(p);
                if (!*p || *p == '}') break;
                if (p[0] == '/' && p[1] == '/') { p = SkipLine(p); linenum++; continue; }

                char key[32];
                if (sscanf(p, "%31s", key) != 1) { p = SkipLine(p); linenum++; continue; }
                p += strlen(key);

                if (strcasecmp(key, "color") == 0)
                    sscanf(p, " %f %f %f", &def.r, &def.g, &def.b);
                else if (strcasecmp(key, "radius") == 0)
                    sscanf(p, " %f", &def.radius);
                else if (strcasecmp(key, "flicker") == 0)
                    sscanf(p, " %f", &def.flicker);

                // Skip to next semicolon or '}'
                while (*p && *p != ';' && *p != '}' && *p != '\n') p++;
                if (*p == ';') p++;
            }
            if (*p == '}') p++;

            namedLights[std::string(lname)] = def;
        }
        else if (strcasecmp(token, "actor") == 0)
        {
            char aname[64], keyword[16], lname[64];
            if (sscanf(p, " %63s %15s %63s", aname, keyword, lname) != 3)
            {
                p = SkipLine(p); linenum++; continue;
            }

            if (strcasecmp(keyword, "light") == 0)
            {
                mobjtype_t mt = FindMobjType(aname);
                if (mt == (mobjtype_t)-1)
                    CONS_Printf("LIGHTDEFS line %d: unknown actor '%s'\n", linenum, aname);
                else
                {
                    std::map<std::string, LightEmitterDef>::iterator it =
                        namedLights.find(std::string(lname));
                    if (it == namedLights.end())
                        CONS_Printf("LIGHTDEFS line %d: unknown light '%s'\n", linenum, lname);
                    else
                    {
                        LightEmitterDef def = it->second;
                        def.type = mt;
                        g_lightEmitters[mt] = def;
                    }
                }
            }
            p = SkipLine(p); linenum++;
        }
        else
        {
            p = SkipLine(p); linenum++;
        }
    }

    Z_Free(buf);
    CONS_Printf("ParseLightDefs: loaded %d light definitions.\n",
                (int)g_lightEmitters.size());
}

bool GetActorLight(const DActor *da, FrameLight &out)
{
    std::map<mobjtype_t, LightEmitterDef>::const_iterator it =
        g_lightEmitters.find(da->type);
    if (it == g_lightEmitters.end())
        return false;

    const LightEmitterDef &def = it->second;
    out.x = da->pos.x.Float();
    out.y = da->pos.y.Float();
    out.z = da->pos.z.Float() + 32.0f;
    out.r = def.r;
    out.g = def.g;
    out.b = def.b;
    out.radius = def.radius;

    // Flicker: vary radius slightly each frame using a hash of the actor address.
    if (def.flicker > 0.0f)
    {
        // Cheap pseudo-random flicker tied to current time.
        unsigned tick = (unsigned)(size_t)da ^ (unsigned)(size_t)(da->state);
        float noise = (float)(tick & 0xFF) / 255.0f;  // 0..1
        out.radius *= 1.0f - def.flicker * noise;
    }

    return true;
}
