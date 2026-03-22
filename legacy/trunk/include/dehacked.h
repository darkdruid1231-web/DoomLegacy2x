// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: dehacked.h 524 2008-01-20 23:48:31Z jussip $
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
/// \brief DeHackEd and BEX support

#ifndef dehacked_h
#define dehacked_h 1

#include "parser.h"

/// \brief DeHackEd patch reader
class dehacked_t
{
  private:
    Parser p;
    int num_errors;

    int deh_pars[4][10];  // BEX [PARS] overrides: pars[episode][level] for Doom1 (1-indexed)
    int deh_cpars[33];    // BEX [PARS] overrides: cpars[mapnum-1] for Doom2 (MAP01-MAP32)
    bool has_pars;        // true if any par overrides were set

    int FindValue();
    int FindState();
    bool ReadFlags(struct mobjinfo_t *m);

    void Read_Thing(const char *str);
    void Read_Frame(const char *str);
    void Read_Sound(int num);
    void Read_Text(int len1, int len2);
    void Read_Weapon(int num);
    void Read_Ammo(int num);
    void Read_Misc();
    void Read_Cheat();
    void Read_CODEPTR();
    void Read_STRINGS();
    void Read_PARS();
    void Read_HELPER();
    void Read_DEFAULTMAP();
    void Read_MAP();
    void Read_CLUSTERDEF();
    void Read_EPISODE();

  public:
    bool loaded;
    int helper_MT;  // [HELPER] substitute thing type (-1 = none)

    dehacked_t();
    bool LoadDehackedLump(int lump);
    void error(const char *first, ...);
    void ApplyPars();  // apply stored [PARS] overrides to MapInfo objects (call after Read_MAPINFO)

    int idfa_armor;
    float idfa_armorfactor;
    int idkfa_armor;
    float idkfa_armorfactor;
    int god_health;

    int max_health;
    int max_soul_health;

    int initial_bullets;
};

extern dehacked_t DEH;

#endif
