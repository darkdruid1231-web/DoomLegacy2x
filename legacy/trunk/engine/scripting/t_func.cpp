// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// $Id: t_func.cpp 522 2008-01-20 22:42:23Z smite-meister $
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2007 Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------

/// \file
/// \brief FS functions
///
/// FS functions are stored as variables, the
/// value being a pointer to a 'handler' function for the
/// function. Arguments are stored in an argc/argv-style list.
///
/// this module contains all the handler functions for the
/// basic FraggleScript Functions.

/*!
  \defgroup g_fs FraggleScript

  FraggleScript guidelines.
*/

/*!
  \page fs_funcs Writing FS functions
  \ingroup g_fs
  To make FS as stable and useful as possible, follow these guidelines:
  - Before adding a new function, think hard if it is actually necessary.
  - Try to make new funcs as general and useful as possible.
    Make them do something useful and logical with all possible argument values.
  - It almost always makes sense for the function to return _something_.
  - Each FS function starts by setting t_return.type (if any).
  - Next, set t_return.value to a value appropriate for an error.
  - Now, check number of arguments. If it's not acceptable, do an informative script_error() and
  return.
  - Before succesfully returning from the function, remember to set t_return.value.
*/

#include <math.h>
#include <stdio.h>

#include "command.h"
#include "cvars.h"
#include "doomtype.h"

#include "g_actor.h"
#include "g_decorate.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_player.h"

#include "g_game.h"
#include "hud.h"

#include "info.h"
#include "m_random.h"

#include "p_maputl.h"
#include "p_spec.h"

#include "r_data.h"
#include "r_main.h"
#include "r_segs.h"

#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "z_zone.h"

#include "p_camera.h"

#include "i_video.h"

#include "t_func.h"
#include "t_parse.h"
#include "t_script.h"
#include "t_spec.h"
#include "t_vari.h"

inline fixed_t AngleToFixed(angle_t x)
{
    return fixed_t(float(x) * 45.0f / ANG45);
}
inline angle_t FixedToAngle(fixed_t x)
{
    return angle_t(x.Float() * ANG45 / 45.0f);
}

svalue_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, char *value);

// functions. SF_ means Script Function not, well.. heh, me

/*******************
  FUNCTIONS
 *******************/

// the actual handler functions for the
// functions themselves

// arguments are evaluated and passed to the
// handler functions using 't_argc' and 't_argv'
// in a similar way to the way C does with command
// line options.

// values can be returned from the functions using
// the variable 't_return'

void SF_Print()
{
    if (!t_argc)
        return;

    // Build full string for HUD display, and print each arg to console
    int strsize = 0;
    for (int i = 0; i < t_argc; i++)
        strsize += strlen(stringvalue(t_argv[i]));

    char *tempstr = (char *)Z_Malloc(strsize + 1, PU_STATIC, 0);
    tempstr[0] = '\0';
    for (int i = 0; i < t_argc; i++)
    {
        const char *s = stringvalue(t_argv[i]);
        CONS_Printf("%s", s);
        strcat(tempstr, s);
    }

    if (trigger_player)
        trigger_player->SetMessage(tempstr, 0, PlayerInfo::M_HUD, 150);

    Z_Free(tempstr);
}

// return a random number from 0 to 255
void SF_Rnd()
{
    t_return.type = svt_int;
    t_return.value.i = rand() % 256;
}

// return a P_Random number from 0 to 255
void SF_PRnd()
{
    t_return.type = svt_int;
    t_return.value.i = P_Random();
}

// looping section. using the rover, find the highest level
// loop we are currently in and return the section_t for it.

fs_section_t *looping_section()
{
    fs_section_t *best = NULL; // highest level loop we're in
                               // that has been found so far
    // check thru all the hashchains
    for (int n = 0; n < SECTIONSLOTS; n++)
    {
        fs_section_t *current = current_script->sections[n];

        // check all the sections in this hashchain
        while (current)
        {
            // a loop?

            if (current->type == st_loop)
                // check to see if it's a loop that we're inside
                if (rover >= current->start && rover <= current->end)
                {
                    // a higher nesting level than the best one so far?
                    if (!best || (current->start > best->start))
                        best = current; // save it
                }
            current = current->next;
        }
    }

    return best; // return the best one found
}

// "continue;" in FraggleScript is a function
void SF_Continue()
{
    fs_section_t *section;

    if (!(section = looping_section())) // no loop found
    {
        script_error("continue() not in loop\n");
        return;
    }

    rover = section->end; // jump to the closing brace
}

void SF_Break()
{
    fs_section_t *section;

    if (!(section = looping_section()))
    {
        script_error("break() not in loop\n");
        return;
    }

    rover = section->end + 1; // jump out of the loop
}

void SF_Goto()
{
    if (t_argc < 1)
    {
        script_error("incorrect arguments to goto\n");
        return;
    }

    // check argument is a labelptr

    if (t_argv[0].type != svt_label)
    {
        script_error("goto argument not a label\n");
        return;
    }

    // go there then if everythings fine

    rover = t_argv[0].value.labelptr;
}

void SF_Return()
{
    killscript = true; // kill the script
}

void SF_Include()
{
    char tempstr[9];

    if (t_argc < 1)
    {
        script_error("incorrect arguments to include()");
        return;
    }

    memset(tempstr, 0, 9);

    if (t_argv[0].type == svt_string)
        strncpy(tempstr, t_argv[0].value.s, 8);
    else
        sprintf(tempstr, "%s", stringvalue(t_argv[0]));

    parse_include(tempstr);
}

void SF_Input()
{
    /*        static char inputstr[128];

                    gets(inputstr);

            t_return.type = svt_string;
            t_return.value.s = inputstr;
    */
    CONS_Printf("input() function not available in doom\a\n");
}

void SF_Beep()
{
    CONS_Printf("\a");
}

void SF_Clock()
{
    t_return.type = svt_int;
    t_return.value.i = (current_map->maptic * 100) / 35;
}

// script function
void SF_Wait()
{
    if (t_argc != 1)
    {
        script_error("incorrect arguments to function\n");
        return;
    }

    current_script->save(rover, wt_delay, (intvalue(t_argv[0]) * 35) / 100);
}

// wait for sector with particular tag to stop moving
void SF_TagWait()
{
    if (t_argc != 1)
    {
        script_error("incorrect arguments to function\n");
        return;
    }

    current_script->save(rover, wt_tagwait, intvalue(t_argv[0]));
}

// wait for a script to finish
void SF_ScriptWait()
{
    if (t_argc != 1)
    {
        script_error("incorrect arguments to function\n");
        return;
    }

    current_script->save(rover, wt_scriptwait, intvalue(t_argv[0]));
}

/**************** doom stuff ****************/
void SF_ExitLevel()
{
    // syntax: exitlevel([nextmap, entrypoint, who])
    int ep = 0;
    int next = 0;
    Actor *who = NULL; // by default everyone exits

    if (t_argc >= 2)
    {
        next = intvalue(t_argv[1]);

        if (t_argc >= 3)
        {
            ep = intvalue(t_argv[2]);

            if (t_argc >= 4)
            {
                int n = intvalue(t_argv[3]);
                PlayerInfo *p = current_map->FindPlayer(n);
                if (!p)
                    p = trigger_player;

                if (p)
                    who = p->pawn;
            }
        }
    }

    current_map->ExitMap(who, next, ep);
}

// centremsg
void SF_Tip()
{
    if (!trigger_player)
        return;

    int i, strsize = 0;

    for (i = 0; i < t_argc; i++)
        strsize += strlen(stringvalue(t_argv[i]));

    char *tempstr = (char *)Z_Malloc(strsize + 1, PU_STATIC, 0);
    tempstr[0] = '\0';

    for (i = 0; i < t_argc; i++)
        sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));

    trigger_player->SetMessage(tempstr, 1, PlayerInfo::M_HUD, 150);
    Z_Free(tempstr);
}

// SoM: Timed tip!
void SF_TimedTip()
{
    if (t_argc < 2)
    {
        script_error("Missing parameters.\n");
        return;
    }

    if (!trigger_player)
        return;

    int tiptime = (intvalue(t_argv[0]) * 35) / 100;

    int i, strsize = 0;

    for (i = 0; i < t_argc; i++)
        strsize += strlen(stringvalue(t_argv[i]));

    char *tempstr = (char *)Z_Malloc(strsize + 1, PU_STATIC, 0);
    tempstr[0] = '\0';

    for (i = 1; i < t_argc; i++)
        sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));

    trigger_player->SetMessage(tempstr, 1, PlayerInfo::M_HUD, tiptime);
    Z_Free(tempstr);
}

// tip to a particular player
void SF_PlayerTip()
{
    if (!t_argc)
    {
        script_error("player not specified\n");
        return;
    }

    PlayerInfo *p = current_map->FindPlayer(intvalue(t_argv[0]) + 1);

    if (!p)
        return;

    int i, strsize = 0;

    for (i = 0; i < t_argc; i++)
        strsize += strlen(stringvalue(t_argv[i]));

    char *tempstr = (char *)Z_Malloc(strsize + 1, PU_STATIC, 0);
    tempstr[0] = '\0';

    for (i = 1; i < t_argc; i++)
        sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));

    p->SetMessage(tempstr, 1, PlayerInfo::M_HUD, 150);
    Z_Free(tempstr);
}

// message player
void SF_Message()
{
    if (!trigger_player)
        return;

    int i, strsize = 0;

    for (i = 0; i < t_argc; i++)
        strsize += strlen(stringvalue(t_argv[i]));

    char *tempstr = (char *)Z_Malloc(strsize + 1, PU_STATIC, 0);
    tempstr[0] = '\0';

    for (i = 0; i < t_argc; i++)
        sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));

    trigger_player->SetMessage(tempstr, 1, PlayerInfo::M_HUD, 150);
    Z_Free(tempstr);
}

// Returns what type of game is going on - Deathmatch, CoOp, or Single Player.
// Feature Requested by SoM! SSNTails 06-13-2002
void SF_GameMode()
{
    t_return.type = svt_int;

    if (cv_deathmatch.value) // Deathmatch!
        t_return.value.i = 2;
    else if (game.multiplayer) // Cooperative
        t_return.value.i = 1;
    else // Single Player
        t_return.value.i = 0;
}

// message to a particular player
void SF_PlayerMsg()
{
    if (!t_argc)
    {
        script_error("player not specified\n");
        return;
    }

    PlayerInfo *p = current_map->FindPlayer(intvalue(t_argv[0]) + 1);

    if (!p)
        return;

    int i, strsize = 0;
    for (i = 0; i < t_argc; i++)
        strsize += strlen(stringvalue(t_argv[i]));

    char *tempstr = (char *)Z_Malloc(strsize + 1, PU_STATIC, 0);
    tempstr[0] = '\0';

    for (i = 1; i < t_argc; i++)
        sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));

    p->SetMessage(tempstr, 1);
    Z_Free(tempstr);
}

void SF_PlayerInGame()
{
    t_return.type = svt_int;

    if (!t_argc)
    {
        script_error("player not specified\n");
        return;
    }

    if (!current_map->FindPlayer(intvalue(t_argv[0]) + 1))
        t_return.value.i = false;
    else
        t_return.value.i = true;
}

void SF_PlayerName()
{
    t_return.type = svt_string;
    t_return.value.s = NULL;

    if (t_argc == 0)
    {
        if (trigger_player)
            t_return.value.s = trigger_player->name.c_str();
        else
        {
            script_error("script not started by player\n");
        }
        return;
    }

    PlayerInfo *pl = current_map->FindPlayer(intvalue(t_argv[0]) + 1);

    if (pl)
        t_return.value.s = pl->name.c_str();
}

// returns the object being controlled by player number n (or NULL)
void SF_PlayerObj()
{
    t_return.type = svt_actor;
    t_return.value.mobj = NULL;

    if (t_argc == 0)
    {
        if (trigger_player)
            t_return.value.mobj = trigger_player->pawn;
        else
        {
            script_error("script not started by player\n");
        }
        return;
    }

    PlayerInfo *pl = current_map->FindPlayer(intvalue(t_argv[0]) + 1);

    if (pl)
        t_return.value.mobj = pl->pawn;
}

void SF_MobjIsPlayer()
{
    t_return.type = svt_int;

    if (t_argc == 0)
    {
        t_return.value.i = trigger_player ? 1 : 0;
        return;
    }

    Actor *mobj = MobjForSvalue(t_argv[0]);
    if (!mobj)
        t_return.value.i = 0;
    else
        t_return.value.i = mobj->Inherits<PlayerPawn>() ? 1 : 0;
}

// returns a valid playerpawn or generates an error
static PlayerPawn *GetPawn(svalue_t &s)
{
    if (s.type == svt_actor)
    {
        Actor *a = s.value.mobj;
        PlayerPawn *p;
        if (!a || !(p = a->Inherits<PlayerPawn>()))
        {
            script_error("mobj not a player!\n");
            return NULL;
        }
        return p;
    }
    else
    {
        int playernum = intvalue(t_argv[0]);
        PlayerInfo *pi = current_map->FindPlayer(playernum + 1);
        if (!pi || !pi->pawn)
        {
            script_error("player %i not in game\n", playernum);
            return NULL;
        }
        return pi->pawn;
    }
}

void SF_PlayerKeys()
{
    t_return.type = svt_int;

    if (t_argc < 2)
    {
        script_error("missing parameters for playerkeys\n");
        return;
    }

    PlayerPawn *p = GetPawn(t_argv[0]);

    int keynum = intvalue(t_argv[1]);
    if (keynum > 5)
    {
        script_error("keynum out of range! %s\n", keynum);
        return;
    }
    keynum += 11; // bypass the Hexen keys...

    if (t_argc == 3)
    {
        int givetake = intvalue(t_argv[2]);
        if (givetake)
            p->keycards |= (1 << keynum);
        else
            p->keycards &= ~(1 << keynum);
    }

    t_return.value.i = p->keycards & (1 << keynum);
}

void SF_PlayerAmmo()
{
    t_return.type = svt_int;

    if (t_argc < 2)
    {
        script_error("missing parameters for playerammo\n");
        return;
    }

    PlayerPawn *p = GetPawn(t_argv[0]);

    int ammonum = intvalue(t_argv[1]);
    if (ammonum >= NUMAMMO || ammonum < 0)
    {
        script_error("ammonum out of range! %s\n", ammonum);
        return;
    }

    if (t_argc == 3)
    {
        int newammo = intvalue(t_argv[2]);
        newammo = newammo > p->maxammo[ammonum] ? p->maxammo[ammonum] : newammo;
        p->ammo[ammonum] = newammo;
    }

    t_return.value.i = p->ammo[ammonum];
}

void SF_MaxPlayerAmmo()
{
    t_return.type = svt_int;

    if (t_argc < 2)
    {
        script_error("missing parameters for maxplayerammo\n");
        return;
    }

    PlayerPawn *p = GetPawn(t_argv[0]);

    int ammonum = intvalue(t_argv[1]);
    if (ammonum >= NUMAMMO || ammonum < 0)
    {
        script_error("maxammonum out of range! %i\n", ammonum);
        return;
    }

    if (t_argc == 3)
    {
        int newmax = intvalue(t_argv[2]);
        p->maxammo[ammonum] = newmax;
    }

    t_return.value.i = p->maxammo[ammonum];
}

extern void SF_StartScript(); // in t_script.c
extern void SF_ScriptRunning();

/*********** Mobj code ***************/

void SF_Player()
{
    t_return.type = svt_int;

    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    PlayerPawn *p;
    if (mo && (p = mo->Inherits<PlayerPawn>()))
    {
        t_return.value.i = p->player->number - 1;
    }
    else
        t_return.value.i = -1;
}

// spawn an object: type, x, y, [angle], [z]
void SF_Spawn()
{
    t_return.type = svt_actor;

    if (t_argc < 3)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    mobjtype_t objtype = mobjtype_t(intvalue(t_argv[0]));

    fixed_t x, y, z;
    x = intvalue(t_argv[1]);
    y = intvalue(t_argv[2]);
    if (t_argc >= 5)
        z = intvalue(t_argv[4]);
    else
    {
        // SoM: Check thing flags for spawn-on-ceiling types...
        z = current_map->GetSubsector(x, y)->sector->floorheight;
    }

    angle_t angle = 0;
    if (t_argc >= 4)
        angle = intvalue(t_argv[3]) * (ANG45 / 45);

    // invalid object to spawn
    if (objtype < 0 || objtype >= NUMMOBJTYPES)
    {
        script_error("unknown object type: %i\n", objtype);
        return;
    }

    DActor *p = current_map->SpawnDActor(x, y, z, objtype);
    p->yaw = angle;

    // TEST, update counters
    if (p->flags & MF_COUNTKILL)
        current_map->kills++;
    if (p->flags & MF_COUNTITEM)
        current_map->items++;

    t_return.value.mobj = p;
}

void SF_SpawnExplosion()
{
    t_return.type = svt_int;

    if (t_argc < 3)
    {
        script_error("SpawnExplosion: Missing arguments\n");
        return;
    }

    mobjtype_t type = mobjtype_t(intvalue(t_argv[0]));
    if (type < 0 || type >= NUMMOBJTYPES)
    {
        script_error("SpawnExplosion: Invalud type number\n");
        return;
    }

    fixed_t x, y, z;
    x = fixedvalue(t_argv[1]);
    y = fixedvalue(t_argv[2]);
    if (t_argc > 3)
        z = fixedvalue(t_argv[3]);
    else
        z = current_map->GetSubsector(x, y)->sector->floorheight;

    DActor *spawn = current_map->SpawnDActor(x, y, z, type);

    t_return.value.i = spawn->SetState(spawn->info->deathstate);
    if (spawn->info->deathsound)
        S_StartSound(spawn, spawn->info->deathsound);
}

void SF_RemoveObj()
{
    if (!t_argc)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    Actor *mo = MobjForSvalue(t_argv[0]);
    if (mo)
        mo->Remove();
}

void SF_KillObj()
{
    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    if (mo)
        mo->Die(NULL, current_script->trigger, 0); // kill it
}

// mobj x, y, z
void SF_ObjX()
{
    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    t_return.type = svt_fixed;
    t_return.value.i = mo ? mo->pos.x.value() : 0; // null ptr check
}

void SF_ObjY()
{
    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    t_return.type = svt_fixed;
    t_return.value.i = mo ? mo->pos.y.value() : 0; // null ptr check
}

void SF_ObjZ()
{
    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    t_return.type = svt_fixed;
    t_return.value.i = mo ? mo->pos.z.value() : 0; // null ptr check
}

// mobj angle
void SF_ObjAngle()
{
    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    t_return.type = svt_fixed;
    t_return.value.i = mo ? AngleToFixed(mo->yaw).value() : 0; // null ptr check
}

// teleport: object, sector_tag
void SF_Teleport()
{
    line_t line; // dummy line for teleport function
    Actor *mo;

    if (t_argc == 0) // no arguments
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    if (t_argc == 1) // 1 argument: sector tag
    {
        mo = current_script->trigger; // default to trigger
        line.tag = intvalue(t_argv[0]);
    }
    else // 2 or more
    {    // teleport a given object
        mo = MobjForSvalue(t_argv[0]);
        line.tag = intvalue(t_argv[1]);
        
        // If MobjForSvalue returns NULL (e.g., mapthing not found or index 0),
        // fall back to the trigger player. This allows scripts to use
        // teleport(0, tag) to teleport player 0 when trigger is NULL
        // (as is the case for levelscripts).
        if (!mo && trigger_player && trigger_player->pawn)
            mo = trigger_player->pawn;
    }

    if (mo)
        current_map->EV_Teleport(line.tag, &line, mo, 1, 0);
}

void SF_SilentTeleport()
{
    line_t line; // dummy line for teleport function
    Actor *mo;

    if (t_argc == 0) // no arguments
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    if (t_argc == 1) // 1 argument: sector tag
    {
        mo = current_script->trigger; // default to trigger
        // If trigger is NULL (e.g., in levelscripts), fall back to trigger_player
        if (!mo && trigger_player && trigger_player->pawn)
            mo = trigger_player->pawn;
        line.tag = intvalue(t_argv[0]);
    }
    else // 2 or more
    {    // teleport a given object
        mo = MobjForSvalue(t_argv[0]);
        line.tag = intvalue(t_argv[1]);
        
        // If MobjForSvalue returns NULL (e.g., mapthing not found or index 0),
        // fall back to the trigger player. This allows scripts to use
        // silentteleport(0, tag) to teleport player 0 when trigger is NULL
        // (as is the case for levelscripts).
        if (!mo && trigger_player && trigger_player->pawn)
            mo = trigger_player->pawn;
    }

    if (mo)
        current_map->EV_Teleport(line.tag, &line, mo, 1, 0x2);
}

void SF_DamageObj()
{
    Actor *mo;
    int damageamount;

    if (t_argc == 0) // no arguments
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    if (t_argc == 1) // 1 argument: damage trigger by amount
    {
        mo = current_script->trigger; // default to trigger
        damageamount = intvalue(t_argv[0]);
    }
    else // 2 or more
    {    // teleport a given object
        mo = MobjForSvalue(t_argv[0]);
        damageamount = intvalue(t_argv[1]);
    }

    if (mo)
        mo->Damage(NULL, current_script->trigger, damageamount);
}

/// the tag number of the sector the thing is in
void SF_ObjSector() // TODO misleading name! ObjSectorTag would be more accurate.
{
    // use trigger object if not specified
    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    t_return.type = svt_int;
    t_return.value.i = mo ? mo->subsector->sector->tag : 0; // nullptr check
}

/// the health of an object
void SF_ObjHealth()
{
    // use trigger object if not specified
    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    t_return.type = svt_int;
    t_return.value.i = mo ? mo->health : 0;
}

void SF_ObjDead()
{
    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    t_return.type = svt_int;
    if (mo && (mo->health <= 0 || mo->flags & MF_CORPSE))
        t_return.value.i = 1;
    else
        t_return.value.i = 0;
}

void SF_ObjFlag()
{
    t_return.type = svt_int;

    // FIXME this function is crap, and should be defined differently. And fixed.
    Actor *mo;
    int f;

    if (t_argc == 0) // no arguments
    {
        script_error("no arguments for function\n");
        return;
    }

    if (t_argc == 1)
    {
        // query flags of trigger object:
        mo = current_script->trigger;
        f = intvalue(t_argv[0]);
    }
    else if (t_argc == 2)
    {
        // query flags of specified object
        mo = MobjForSvalue(t_argv[0]);
        f = intvalue(t_argv[1]);
    }
    else // >= 3 : SET flags of specified object
    {
        mo = MobjForSvalue(t_argv[0]);
        f = intvalue(t_argv[1]);

        if (mo)
        {
            int newflag;
            // remove old bit
            mo->flags = mo->flags & ~(1 << f);

            // make the new flag
            newflag = (!!intvalue(t_argv[2])) << f;
            mo->flags |= newflag; // add new flag to mobj flags
        }
    }

    // nullptr check:
    t_return.value.i = mo ? !!(mo->flags & (1 << f)) : 0;
}

// apply momentum to a thing
void SF_PushThing()
{
    if (t_argc < 3) // missing arguments
    {
        script_error("insufficient arguments for function\n");
        return;
    }

    Actor *mo = MobjForSvalue(t_argv[0]);

    if (!mo)
        return;

    angle_t angle = FixedToAngle(fixedvalue(t_argv[1]));
    fixed_t force = fixedvalue(t_argv[2]);

    mo->vel.x += Cos(angle) * force;
    mo->vel.y += Sin(angle) * force;
}

void SF_ReactionTime()
{
    t_return.type = svt_int;
    t_return.value.i = 0;

    if (t_argc < 1)
    {
        script_error("no arguments for function\n");
        return;
    }

    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
        return;

    if (t_argc > 1)
    {
        mo->reactiontime = (intvalue(t_argv[1]) * 35) / 100;
    }

    t_return.value.i = mo->reactiontime;
}

// Sets a mobj's Target! >:)
void SF_MobjTarget()
{
    t_return.type = svt_actor;
    t_return.value.mobj = NULL;

    if (t_argc < 1)
    {
        script_error("Missing parameters!\n");
        return;
    }

    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
        return;

    if (t_argc >= 2)
    {
        if (t_argv[1].type != svt_actor && intvalue(t_argv[1]) == -1)
        {
            // Set target to NULL
            mo->target = NULL;
            // mo->SetState(mo->info->spawnstate);
        }
        else
        {
            mo->target = MobjForSvalue(t_argv[1]);
            // mo->SetState(mo->info->seestate);
        }
    }

    t_return.value.mobj = mo->target;
}

void SF_MobjMomx()
{
    t_return.type = svt_fixed;
    t_return.value.i = 0;

    if (t_argc < 1)
    {
        script_error("missing parameters\n");
        return;
    }

    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
        return;

    if (t_argc > 1)
    {
        mo->vel.x = fixedvalue(t_argv[1]);
    }

    t_return.value.i = mo->vel.x.value();
}

void SF_MobjMomy()
{
    t_return.type = svt_fixed;
    t_return.value.i = 0;

    if (t_argc < 1)
    {
        script_error("missing parameters\n");
        return;
    }

    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
        return;

    if (t_argc > 1)
    {
        mo->vel.y = fixedvalue(t_argv[1]);
    }

    t_return.value.i = mo->vel.y.value();
}

void SF_MobjMomz()
{
    t_return.type = svt_fixed;
    t_return.value.i = 0;

    if (t_argc < 1)
    {
        script_error("missing parameters\n");
        return;
    }

    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
        return;

    if (t_argc > 1)
    {
        mo->vel.z = fixedvalue(t_argv[1]);
    }

    t_return.value.i = mo->vel.z.value();
}

/****************** Trig *********************/

void SF_PointToAngle()
{
    t_return.type = svt_fixed;

    if (t_argc < 4)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    fixed_t x1, y1, x2, y2;
    x1 = intvalue(t_argv[0]);
    y1 = intvalue(t_argv[1]);
    x2 = intvalue(t_argv[2]);
    y2 = intvalue(t_argv[3]);

    angle_t angle = R_PointToAngle2(x1, y1, x2, y2);
    t_return.value.i = AngleToFixed(angle).value();
}

void SF_PointToDist()
{
    t_return.type = svt_fixed;

    if (t_argc < 4)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    fixed_t x1, x2, y1, y2;
    x1 = intvalue(t_argv[0]);
    y1 = intvalue(t_argv[1]);
    x2 = intvalue(t_argv[2]);
    y2 = intvalue(t_argv[3]);

    t_return.value.i = R_PointToDist2(x1, y1, x2, y2).value();
}

/************* Camera functions ***************/

angle_t G_ClipAimingPitch(angle_t pitch);

/// setcamera(obj, [angle], [viewheight], [aiming])
/// Activates and orients a camera.
void SF_SetCamera()
{
    t_return.type = svt_fixed;

    // The camera system uses special mapthings (DoomEd number 5003) as markers for
    // initial camera locations, and later as anchorpoints for the camera Actors.

    if (t_argc < 1)
    {
        script_error("insufficient arguments for SetCamera.\n");
        return;
    }

    Actor *cam = MobjForSvalue(t_argv[0]);
    if (!cam)
    {
        script_error("SetCamera: No camera object found.\n");
        return;
    }

    // Why do we reset the angle but not the position?
    cam->yaw =
        (t_argc < 2) ? ANG45 * (cam->spawnpoint->angle / 45) : FixedToAngle(fixedvalue(t_argv[1]));

    // Why an absolute height value and not height above floor?
    cam->pos.z = (t_argc < 3) ? (cam->subsector->sector->floorheight + 41) : fixedvalue(t_argv[2]);

    angle_t aiming = (t_argc < 4) ? 0 : FixedToAngle(fixedvalue(t_argv[3]));
    cam->pitch = G_ClipAimingPitch(aiming);

    // Camera has been created and oriented. Now use it as POV for players:

    int n = current_map->players.size();
    for (int i = 0; i < n; i++)
        current_map->players[i]->pov = cam; // FIXME loses personal chasecams!

    t_return.value.i = cam->pitch;
}

/// Shuts down all cameras in the Map.
void SF_ClearCamera()
{
    int n = current_map->players.size();
    for (int i = 0; i < n; i++)
        current_map->players[i]->pov =
            current_map->players[i]
                ->pawn; // FIXME return the original POVs for all players (chasecams!)
}

// movecamera(cameraobj, targetobj, targetheight, movespeed, targetangle, anglespeed)
void SF_MoveCamera()
{
    t_return.type = svt_int;

    fixed_t x, y, z;
    fixed_t xdist, ydist, zdist, xydist, movespeed;
    fixed_t xstep, ystep, zstep, targetheight;
    angle_t anglespeed, anglestep = 0, angledist, targetangle, mobjangle, bigangle, smallangle;
    // I have to use floats for the math where angles are divided by fixed
    // values.
    double fangledist, fanglestep, fmovestep;
    int angledir = 0;
    Actor *camera;
    Actor *target;
    int moved = 0;
    int quad1, quad2;

    if (t_argc < 6)
    {
        script_error("movecamera: insufficient arguments to function\n");
        return;
    }

    camera = MobjForSvalue(t_argv[0]);
    target = MobjForSvalue(t_argv[1]);
    targetheight = fixedvalue(t_argv[2]);
    movespeed = fixedvalue(t_argv[3]);
    targetangle = FixedToAngle(fixedvalue(t_argv[4]));
    anglespeed = FixedToAngle(fixedvalue(t_argv[5]));

    // Figure out how big the step will be
    xdist = target->pos.x - camera->pos.x;
    ydist = target->pos.y - camera->pos.y;
    zdist = targetheight - camera->pos.z;

    // Angle checking...
    //    90
    //   Q1|Q0
    // 180--+--0
    //   Q2|Q3
    //    270
    quad1 = targetangle / ANG90;
    quad2 = camera->yaw / ANG90;
    bigangle = targetangle > camera->yaw ? targetangle : camera->yaw;
    smallangle = targetangle < camera->yaw ? targetangle : camera->yaw;
    if ((quad1 > quad2 && quad1 - 1 == quad2) || (quad2 > quad1 && quad2 - 1 == quad1) ||
        quad1 == quad2)
    {
        angledist = bigangle - smallangle;
        angledir = targetangle > camera->yaw ? 1 : -1;
    }
    else
    {
        if (quad2 == 3 && quad1 == 0)
        {
            angledist = (bigangle + ANG180) - (smallangle + ANG180);
            angledir = 1;
        }
        else if (quad1 == 3 && quad2 == 0)
        {
            angledist = (bigangle + ANG180) - (smallangle + ANG180);
            angledir = -1;
        }
        else
        {
            angledist = bigangle - smallangle;
            if (angledist > ANG180)
            {
                angledist = (bigangle + ANG180) - (smallangle + ANG180);
                angledir = targetangle > camera->yaw ? -1 : 1;
            }
            else
                angledir = targetangle > camera->yaw ? 1 : -1;
        }
    }

    // CONS_Printf("angle: cam=%i, target=%i; dir: %i; quads: 1=%i, 2=%i\n", camera->yaw / ANGLE_1,
    // targetangle / ANGLE_1, angledir, quad1, quad2);
    //  set the step variables based on distance and speed...
    mobjangle = R_PointToAngle2(camera->pos, target->pos);

    if (movespeed != 0)
    {
        xydist = R_PointToDist2(camera->pos.x, camera->pos.y, target->pos.x, target->pos.y);
        xstep = Cos(mobjangle) * movespeed;
        ystep = Sin(mobjangle) * movespeed;
        if (xydist != 0)
            zstep = zdist / (xydist / movespeed);
        else
            zstep = zdist > 0 ? movespeed : -movespeed;

        if (xydist != 0 && !anglespeed)
        {
            fangledist = double(angledist) / ANGLE_1;
            fmovestep = (xydist / movespeed).Float();
            if (fmovestep)
                fanglestep = (fangledist / fmovestep);
            else
                fanglestep = 360;

            // CONS_Printf("fstep: %f, fdist: %f, fmspeed: %f, ms: %i\n", fanglestep, fangledist,
            // fmovestep, FixedDiv(xydist, movespeed) >> FRACBITS);

            anglestep = angle_t(fanglestep * ANGLE_1);
        }
        else
            anglestep = anglespeed;

        if (abs(xstep) >= (abs(xdist) - 1))
            x = target->pos.x;
        else
        {
            x = camera->pos.x + xstep;
            moved = 1;
        }

        if (abs(ystep) >= (abs(ydist) - 1))
            y = target->pos.y;
        else
        {
            y = camera->pos.y + ystep;
            moved = 1;
        }

        if (abs(zstep) >= abs(zdist) - 1)
            z = targetheight;
        else
        {
            z = camera->pos.z + zstep;
            moved = 1;
        }
    }
    else
    {
        x = camera->pos.x;
        y = camera->pos.y;
        z = camera->pos.z;
    }

    if (anglestep >= angledist)
        camera->yaw = targetangle;
    else
    {
        if (angledir == 1)
        {
            moved = 1;
            camera->yaw += anglestep;
        }
        else if (angledir == -1)
        {
            moved = 1;
            camera->yaw -= anglestep;
        }
    }

    if ((x != camera->pos.x || y != camera->pos.y) && !camera->TryMove(x, y, true).first)
    {
        script_error("Illegal camera move\n");
        return;
    }
    camera->pos.z = z;

    t_return.value.i = moved;
}

/*********** sounds ******************/

// start sound from thing
void SF_StartSound()
{
    if (t_argc < 2)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    if (t_argv[1].type != svt_string)
    {
        script_error("sound lump argument not a string!\n");
        return;
    }

    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
        return;

    soundsource_t s;
    s.isactor = true;
    s.act = mo;

    sfxinfo_t temp("FS sound", -1);
    strncpy(temp.lumpname, t_argv[1].value.s, 8);

    S.Start3DSound(&temp, &s);
}

// start sound from sector
void SF_StartSectorSound()
{
    if (t_argc < 2)
    {
        script_error("insufficient arguments to function\n");
        return;
    }
    if (t_argv[1].type != svt_string)
    {
        script_error("sound lump argument not a string!\n");
        return;
    }

    int tagnum = intvalue(t_argv[0]);
    // argv is sector tag

    int secnum = current_map->FindSectorFromTag(tagnum, -1);

    if (secnum < 0)
    {
        script_error("sector not found with tagnum %i\n", tagnum);
        return;
    }

    sfxinfo_t temp("FS sound", -1);
    strncpy(temp.lumpname, t_argv[1].value.s, 8);

    secnum = -1;
    while ((secnum = current_map->FindSectorFromTag(tagnum, secnum)) >= 0)
    {
        mappoint_t *p = &current_map->sectors[secnum].soundorg;
        soundsource_t s;
        s.isactor = false;
        s.mpt = p;

        S.Start3DSound(&temp, &s);
    }
}

// backwards compatibility (sound name automatically prefixed with "ds")
void SF_AmbientSound()
{
    if (t_argc != 1)
    {
        script_error("insufficient arguments to function\n");
        return;
    }
    if (t_argv[0].type != svt_string)
    {
        script_error("sound lump argument not a string!\n");
        return;
    }

    sfxinfo_t temp("FS sound", -1);
    strncpy(temp.lumpname, "ds", 2);
    strncpy(&temp.lumpname[2], t_argv[0].value.s, 6);

    S.StartAmbSound(&temp);
}

void SF_StartAmbientSound()
{
    if (t_argc != 1)
    {
        script_error("insufficient arguments to function\n");
        return;
    }
    if (t_argv[0].type != svt_string)
    {
        script_error("sound lump argument not a string!\n");
        return;
    }

    sfxinfo_t temp("FS sound", -1);
    strncpy(temp.lumpname, t_argv[0].value.s, 8);

    S.StartAmbSound(&temp);
}

/************* Sector functions ***************/

// floor height of sector
void SF_FloorHeight()
{
    t_return.type = svt_int;

    if (!t_argc)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    int tagnum = intvalue(t_argv[0]);

    // argv is sector tag
    int secnum = current_map->FindSectorFromTag(tagnum, -1);

    if (secnum < 0)
    {
        script_error("sector not found with tagnum %i\n", tagnum);
        return;
    }

    sector_t *s = &current_map->sectors[secnum];

    int returnval = 1;

    if (t_argc > 1) // > 1: set floorheight
    {
        int i = -1;
        int crush = (t_argc == 3) ? intvalue(t_argv[2]) : 0;

        // set all sectors with tag
        while ((i = current_map->FindSectorFromTag(tagnum, i)) >= 0)
        {
            s = &current_map->sectors[i];
            if (current_map->T_MovePlane(
                    s, fixedvalue(t_argv[1]) - s->floorheight, fixedvalue(t_argv[1]), crush, 0) ==
                res_crushed)
                returnval = 0;
        }
    }
    else
        returnval = s->floorheight.floor();

    t_return.value.i = returnval;
}

void SF_MoveFloor()
{
    int secnum = -1;
    sector_t *sec;

    if (t_argc < 2)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    int tagnum = intvalue(t_argv[0]);
    fixed_t destheight = intvalue(t_argv[1]);
    fixed_t platspeed = FLOORSPEED * (t_argc > 2 ? intvalue(t_argv[2]) : 1);

    // move all sectors with tag

    while ((secnum = current_map->FindSectorFromTag(tagnum, secnum)) >= 0)
    {
        sec = &current_map->sectors[secnum];

        // Don't start a second thinker on the same floor
        if (sec->Active(sector_t::floor_special))
            continue;

        new floor_t(current_map, floor_t::AbsHeight, sec, platspeed, 0, destheight);
    }
}

// ceiling height of sector
void SF_CeilingHeight()
{
    t_return.type = svt_int;

    if (!t_argc)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    int tagnum = intvalue(t_argv[0]);

    // argv is sector tag
    int secnum = current_map->FindSectorFromTag(tagnum, -1);

    if (secnum < 0)
    {
        script_error("sector not found with tagnum %i\n", tagnum);
        return;
    }

    sector_t *s = &current_map->sectors[secnum];

    int returnval = 1;

    if (t_argc > 1) // > 1: set ceilheight
    {
        int i = -1;
        int crush = (t_argc == 3) ? intvalue(t_argv[2]) : 0;

        // set all sectors with tag
        while ((i = current_map->FindSectorFromTag(tagnum, i)) >= 0)
        {
            s = &current_map->sectors[i];
            if (current_map->T_MovePlane(
                    s, fixedvalue(t_argv[1]) - s->ceilingheight, fixedvalue(t_argv[1]), crush, 1) ==
                res_crushed)
                returnval = 0;
        }
    }
    else
        returnval = s->ceilingheight.floor();

    // return floorheight
    t_return.value.i = returnval;
}

void SF_MoveCeiling()
{
    int secnum = -1;
    sector_t *sec;
    ceiling_t *ceiling;

    if (t_argc < 2)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    int tagnum = intvalue(t_argv[0]);
    fixed_t destheight = intvalue(t_argv[1]);
    fixed_t platspeed = FLOORSPEED * (t_argc > 2 ? intvalue(t_argv[2]) : 1);

    // move all sectors with tag

    while ((secnum = current_map->FindSectorFromTag(tagnum, secnum)) >= 0)
    {
        sec = &current_map->sectors[secnum];

        // Don't start a second thinker on the same floor
        if (sec->Active(sector_t::ceiling_special))
            continue;

        ceiling = new ceiling_t(current_map, ceiling_t::AbsHeight, sec, platspeed, 0, destheight);
        current_map->AddActiveCeiling(ceiling);
    }
}

void SF_LightLevel()
{
    if (!t_argc)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    int tagnum = intvalue(t_argv[0]);

    // argv is sector tag
    int secnum = current_map->FindSectorFromTag(tagnum, -1);

    if (secnum < 0)
    {
        script_error("sector not found with tagnum %i\n", tagnum);
        return;
    }

    sector_t *s = &current_map->sectors[secnum];

    if (t_argc > 1) // > 1: set ceilheight
    {
        int i = -1;
        int ll = intvalue(t_argv[1]);

        // set all sectors with tag
        while ((i = current_map->FindSectorFromTag(tagnum, i)) >= 0)
            current_map->sectors[i].lightlevel = ll;
    }

    // return lightlevel
    t_return.type = svt_int;
    t_return.value.i = s->lightlevel;
}

void SF_FadeLight()
{
    int sectag, destlevel, speed = 1;

    if (t_argc < 2)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    sectag = intvalue(t_argv[0]);
    destlevel = intvalue(t_argv[1]);
    speed = t_argc > 2 ? intvalue(t_argv[2]) : 1;

    current_map->EV_FadeLight(sectag, destlevel, speed);
}

void SF_FloorTexture()
{
    if (!t_argc)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    int tagnum = intvalue(t_argv[0]);

    // argv is sector tag
    int secnum = current_map->FindSectorFromTag(tagnum, -1);

    if (secnum < 0)
    {
        script_error("sector not found with tagnum %i\n", tagnum);
        return;
    }

    sector_t *s = &current_map->sectors[secnum];

    if (t_argc > 1)
    {
        int i = -1;
        Material *pic = materials.Get(t_argv[1].value.s, TEX_floor);

        // set all sectors with tag
        while ((i = current_map->FindSectorFromTag(tagnum, i)) >= 0)
            current_map->sectors[i].floorpic = pic;
    }

    t_return.type = svt_string;
    t_return.value.s = s->floorpic ? Z_Strdup(s->floorpic->GetName(), PU_STATIC, 0) : "sky";
}

void SF_SectorColormap()
{
    if (!t_argc)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    int tagnum = intvalue(t_argv[0]);

    // argv is sector tag
    int secnum = current_map->FindSectorFromTag(tagnum, -1);

    if (secnum < 0)
    {
        script_error("sector not found with tagnum %i\n", tagnum);
        return;
    }

    sector_t *s = &current_map->sectors[secnum];

    if (t_argc > 1)
    {
        int i = -1;
        fadetable_t *cmap = R_FindColormap(t_argv[1].value.s);
        if (!cmap)
        {
            script_error("colormap %s not found\n", t_argv[1].value.s);
            return;
        }

        // set all sectors with tag
        while ((i = current_map->FindSectorFromTag(tagnum, i)) >= 0)
        {
            sector_t *p = &current_map->sectors[i];
            p->midmap = cmap;
        }
    }

    t_return.type = svt_string;
    t_return.value.s = R_ColormapNameForNum(s->midmap);
}

void SF_CeilingTexture()
{
    if (!t_argc)
    {
        script_error("insufficient arguments to function\n");
        return;
    }

    int tagnum = intvalue(t_argv[0]);

    // argv is sector tag
    int secnum = current_map->FindSectorFromTag(tagnum, -1);

    if (secnum < 0)
    {
        script_error("sector not found with tagnum %i\n", tagnum);
        return;
    }

    sector_t *s = &current_map->sectors[secnum];

    if (t_argc > 1)
    {
        int i = -1;
        Material *pic = materials.Get(t_argv[1].value.s, TEX_floor);

        // set all sectors with tag
        while ((i = current_map->FindSectorFromTag(tagnum, i)) >= 0)
            current_map->sectors[i].ceilingpic = pic;
    }

    t_return.type = svt_string;
    t_return.value.s = s->ceilingpic ? Z_Strdup(s->ceilingpic->GetName(), PU_STATIC, 0) : "sky";
}

void SF_ChangeHubLevel()
{
    /*  int tagnum;

      if(!t_argc)
        {
          script_error("hub level to go to not specified!\n");
          return;
        }
      if(t_argv[0].type != svt_string)
        {
          script_error("level argument is not a string!\n");
          return;
        }

      // second argument is tag num for 'seamless' travel
      if(t_argc > 1)
        tagnum = intvalue(t_argv[1]);
      else
        tagnum = -1;

      P_SavePlayerPosition(current_script->trigger->player, tagnum);
      P_ChangeHubLevel(t_argv[0].value.s);*/
}

// for start map: start new game on a particular skill
void SF_StartSkill()
{
    if (t_argc < 1)
    {
        script_error("need skill level to start on\n");
        return;
    }

    // -1: 1-5 is how we normally see skills
    // 0-4 is how doom sees them

    int s = intvalue(t_argv[0]) - 1;
    if (s < 0 || s > 4)
    {
        script_error("skill level out of bounds\n");
        return;
    }

    game.SV_StartGame(skill_t(s), 1);
}

//////////////////////////////////////////////////////////////////////////
//
// Doors
//

// opendoor(sectag, [delay], [speed])

void SF_OpenDoor()
{
    int speed, wait_time;
    int sectag;

    if (t_argc < 1)
    {
        script_error("need sector tag for door to open\n");
        return;
    }

    // got sector tag
    sectag = intvalue(t_argv[0]);

    // door wait time

    if (t_argc > 1) // door wait time
        wait_time = (intvalue(t_argv[1]) * 35) / 100;
    else
        wait_time = 0; // 0= stay open

    // door speed

    if (t_argc > 2)
        speed = intvalue(t_argv[2]);
    else
        speed = 1; // 1= normal speed

    current_map->EV_OpenDoor(sectag, speed, wait_time);
}

void SF_CloseDoor()
{
    int speed;
    int sectag;

    if (t_argc < 1)
    {
        script_error("need sector tag for door to open\n");
        return;
    }

    // got sector tag
    sectag = intvalue(t_argv[0]);

    // door speed

    if (t_argc > 1)
        speed = intvalue(t_argv[1]);
    else
        speed = 1; // 1= normal speed

    current_map->EV_CloseDoor(sectag, speed);
}

// run console cmd

void SF_RunCommand()
{
    int i;
    char *tempstr;
    int strsize = 0;

    for (i = 0; i < t_argc; i++)
        strsize += strlen(stringvalue(t_argv[i]));

    tempstr = (char *)Z_Malloc(strsize + 1, PU_STATIC, 0);
    tempstr[0] = '\0';

    for (i = 0; i < t_argc; i++)
        sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));

    COM.AppendText(tempstr);
    Z_Free(tempstr);
}

// any linedef type
void ConvertLineDef(line_t *ld);

void SF_LineTrigger()
{
    if (!t_argc)
    {
        script_error("need line trigger type\n");
        return;
    }

    line_t junk; // FIXME very dangerous (contains pointers which are not initialized!)
    memset(&junk, 0, sizeof(line_t));

    junk.special = intvalue(t_argv[0]);
    junk.tag = (t_argc < 2) ? 0 : intvalue(t_argv[1]);

    // current_map->UseSpecialLine(t_trigger, &junk, 0);    // Try using it
    // current_map->ActivateCrossedLine(&junk, 0, t_trigger);   // Try crossing it

    ConvertLineDef(&junk); // to corresponding Hexen linedef

    current_map->ActivateLine(&junk, trigger_obj, 0, SPAC_USE);
    current_map->ActivateLine(&junk, trigger_obj, 0, SPAC_CROSS);
}

void SF_LineFlag()
{
    t_return.type = svt_int;

    if (t_argc < 2)
    {
        script_error("LineFlag: missing parameters\n");
        return;
    }

    int linenum = intvalue(t_argv[0]);

    if (linenum < 0 || linenum > current_map->numlines)
    {
        script_error("LineFlag: Invalid line number.\n");
        return;
    }

    line_t *line = current_map->lines + linenum;

    int flagnum = intvalue(t_argv[1]);
    if (flagnum < 0 || flagnum > 31)
    {
        script_error("LineFlag: Invalid flag number.\n");
        return;
    }

    if (t_argc > 2)
    {
        line->flags &= ~(1 << flagnum);
        if (intvalue(t_argv[2]))
            line->flags |= (1 << flagnum);
    }

    t_return.value.i = line->flags & (1 << flagnum);
}

void SF_ChangeMusic()
{
    if (!t_argc)
    {
        script_error("need new music name\n");
        return;
    }
    if (t_argv[0].type != svt_string)
    {
        script_error("incorrect argument to function\n");
        return;
    }

    S.StartMusic(t_argv[0].value.s, true);
}

// SoM: Max and Min math functions.
void SF_Max()
{
    fixed_t n1, n2;

    if (t_argc != 2)
    {
        script_error("invalid number of arguments\n");
        return;
    }

    n1 = fixedvalue(t_argv[0]);
    n2 = fixedvalue(t_argv[1]);

    t_return.type = svt_fixed;
    t_return.value.i = n1 > n2 ? n1.value() : n2.value();
}

void SF_Min()
{
    fixed_t n1, n2;

    if (t_argc != 2)
    {
        script_error("invalid number of arguments\n");
        return;
    }

    n1 = fixedvalue(t_argv[0]);
    n2 = fixedvalue(t_argv[1]);

    t_return.type = svt_fixed;
    t_return.value.i = n1 < n2 ? n1.value() : n2.value();
}

void SF_Abs()
{
    fixed_t n1;

    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
        return;
    }

    n1 = fixedvalue(t_argv[0]);

    t_return.type = svt_fixed;
    t_return.value.i = n1 < 0 ? (-n1).value() : n1.value();
}

// Hurdler: some new math functions
static int double2fixed(double t)
{
    /*
    double fl = floor(t);
    return ((int)fl << 16) | (int)((t-fl)*65536.0);
    */
    return fixed_t(float(t)).value();
}

void SF_Sin()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = double2fixed(sin(n1.Float()));
    }
}

void SF_ASin()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = double2fixed(asin(n1.Float()));
    }
}

void SF_Cos()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = double2fixed(cos(n1.Float()));
    }
}

void SF_ACos()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = double2fixed(acos(n1.Float()));
    }
}

void SF_Tan()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = double2fixed(tan(n1.Float()));
    }
}

void SF_ATan()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = double2fixed(atan(n1.Float()));
    }
}

void SF_Exp()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = double2fixed(exp(n1.Float()));
    }
}

void SF_Log()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = double2fixed(log(n1.Float()));
    }
}

void SF_Sqrt()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = double2fixed(sqrt(n1.Float()));
    }
}

void SF_Floor()
{
    if (t_argc != 1)
    {
        script_error("invalid number of arguments\n");
    }
    else
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = svt_fixed;
        t_return.value.i = n1.value() & 0xFFFF0000;
    }
}

void SF_Pow()
{
    fixed_t n1, n2;
    if (t_argc != 2)
    {
        script_error("invalid number of arguments\n");
        return;
    }
    n1 = fixedvalue(t_argv[0]);
    n2 = fixedvalue(t_argv[1]);
    t_return.type = svt_fixed;
    t_return.value.i = double2fixed(pow(n1.Float(), n2.Float()));
}

//////////////////////////////////////////////////////////////////////////
// FraggleScript HUD graphics
//////////////////////////////////////////////////////////////////////////

void SF_NewHUPic()
{
    if (t_argc != 3)
    {
        script_error("newhupic: invalid number of arguments\n");
        return;
    }

    t_return.type = svt_int;
    t_return.value.i = hud.GetFSPic(
        fc.GetNumForName(stringvalue(t_argv[0])), intvalue(t_argv[1]), intvalue(t_argv[2]));
    return;
}

void SF_DeleteHUPic()
{
    if (t_argc != 1)
    {
        script_error("deletehupic: Invalid number if arguments\n");
        return;
    }

    if (hud.DeleteFSPic(intvalue(t_argv[0])))
        script_error("deletehupic: Invalid sfpic handle: %i\n", intvalue(t_argv[0]));
    return;
}

void SF_ModifyHUPic()
{
    if (t_argc != 4)
    {
        script_error("modifyhupic: invalid number of arguments\n");
        return;
    }

    if (hud.ModifyFSPic(intvalue(t_argv[0]),
                        fc.GetNumForName(stringvalue(t_argv[1])),
                        intvalue(t_argv[2]),
                        intvalue(t_argv[3])))
        script_error("modifyhypic: invalid sfpic handle %i\n", intvalue(t_argv[0]));
    return;
}

void SF_SetHUPicDisplay()
{
    if (t_argc != 2)
    {
        script_error("sethupicdisplay: invalud number of arguments\n");
        return;
    }

    if (hud.DisplayFSPic(intvalue(t_argv[0]), intvalue(t_argv[1]) > 0 ? true : false))
        script_error("sethupicdisplay: invalid pic handle %i\n", intvalue(t_argv[0]));
}

// Hurdler: I'm enjoying FS capability :)

int String2Hex(const char *s)
{
#define HEX2INT(x)                                                                                 \
    (x >= '0' && x <= '9'   ? x - '0'                                                              \
     : x >= 'a' && x <= 'f' ? x - 'a' + 10                                                         \
     : x >= 'A' && x <= 'F' ? x - 'A' + 10                                                         \
                            : 0)
    return (HEX2INT(s[0]) << 4) + (HEX2INT(s[1]) << 0) + (HEX2INT(s[2]) << 12) +
           (HEX2INT(s[3]) << 8) + (HEX2INT(s[4]) << 20) + (HEX2INT(s[5]) << 16) +
           (HEX2INT(s[6]) << 28) + (HEX2INT(s[7]) << 24);
#undef HEX2INT
}

void SF_SetCorona()
{
#warning setcorona: Hurdler, must be uncommented once the new dynamic light code is OK
#if 0

    if (rendermode == render_soft)
        return; // do nothing in software mode
    if (t_argc != 3 && t_argc != 7)
    {
        script_error("Incorrect parameters.\n");
        return;
    }
    //this function accept 2 kinds of parameters
    if (t_argc == 3)
    {
        int    num  = t_argv[0].value.i; // which corona we want to modify
        int    what = t_argv[1].value.i; // what we want to modify (type, color, offset,...)
        int    ival = t_argv[2].value.i; // the value of what we modify
        double fval = ((double)t_argv[3].value.f / FRACUNIT);

        switch (what)
        {
        case 0: lspr[num].type = ival; break;
        case 1: lspr[num].light_xoffset = fval; break;
        case 2: lspr[num].light_yoffset = fval; break;
        case 3:
            if (t_argv[2].type == svt_string)
                lspr[num].corona_color = String2Hex(t_argv[2].value.s);
            else
                memcpy(&lspr[num].corona_color, &ival, sizeof(int));
            break;
        case 4: lspr[num].corona_radius = fval; break;
        case 5:
            if (t_argv[2].type == svt_string)
                lspr[num].dynamic_color = String2Hex(t_argv[2].value.s);
            else
                memcpy(&lspr[num].dynamic_color, &ival, sizeof(int));
            break;
        case 6:
            lspr[num].dynamic_radius = fval;
            lspr[num].dynamic_sqrradius = sqrt(lspr[num].dynamic_radius);
            break;
        default: CONS_Printf("Error in setcorona\n"); break;
        }
    }
    else
    {
        int num = t_argv[0].value.i; // which corona we want to modify
        lspr[num].type          = t_argv[1].value.i;
        lspr[num].light_xoffset = t_argv[2].value.f;
        lspr[num].light_yoffset = t_argv[3].value.f;
        if (t_argv[4].type == svt_string)
            lspr[num].corona_color = String2Hex(t_argv[4].value.s);
        else
            memcpy(&lspr[num].corona_color, &t_argv[4].value.i, sizeof(int));
        lspr[num].corona_radius = t_argv[5].value.f;
        if (t_argv[6].type == svt_string)
            lspr[num].dynamic_color = String2Hex(t_argv[6].value.s);
        else
            memcpy(&lspr[num].dynamic_color, &t_argv[6].value.i, sizeof(int));
        lspr[num].dynamic_radius = t_argv[7].value.f;
        lspr[num].dynamic_sqrradius = sqrt(lspr[num].dynamic_radius);
    }
#endif // if 0
}

//////////////////////////////////////////////////////////////////////////
// Array functions - ported from legacy_one
//////////////////////////////////////////////////////////////////////////

void SF_NewArray()
{
    // newarray(initial_values...)
    // Creates an array with the given initial values
    t_return.type = svt_array;
    t_return.value.array = NULL;
    
    if (t_argc < 1)
    {
        // Create empty array
        fs_array_t *arr = (fs_array_t *)Z_Malloc(sizeof(fs_array_t), PU_STATIC, 0);
        arr->next = NULL;
        arr->saveindex = 0;
        arr->length = 0;
        arr->values = NULL;
        t_return.value.array = arr;
        return;
    }
    
    fs_array_t *arr = (fs_array_t *)Z_Malloc(sizeof(fs_array_t), PU_STATIC, 0);
    arr->next = NULL;
    arr->saveindex = 0;
    arr->length = t_argc;
    arr->values = (svalue_t *)Z_Malloc(sizeof(svalue_t) * t_argc, PU_STATIC, 0);
    
    for (int i = 0; i < t_argc; i++)
    {
        arr->values[i] = t_argv[i];
    }
    
    t_return.value.array = arr;
}

void SF_NewEmptyArray()
{
    // newemptyarray(length)
    // Creates an empty array of specified length
    t_return.type = svt_array;
    t_return.value.array = NULL;
    
    if (t_argc < 1)
    {
        script_error("newemptyarray: requires length argument\n");
        return;
    }
    
    int len = intvalue(t_argv[0]);
    if (len < 0)
    {
        script_error("newemptyarray: length must be positive\n");
        return;
    }
    
    fs_array_t *arr = (fs_array_t *)Z_Malloc(sizeof(fs_array_t), PU_STATIC, 0);
    arr->next = NULL;
    arr->saveindex = 0;
    arr->length = len;
    arr->values = (svalue_t *)Z_Malloc(sizeof(svalue_t) * len, PU_STATIC, 0);
    
    // Initialize with null values
    for (int i = 0; i < len; i++)
    {
        arr->values[i].type = svt_int;
        arr->values[i].value.i = 0;
    }
    
    t_return.value.array = arr;
}

void SF_ArrayCopyInto()
{
    // copyinto(source_array, dest_array)
    // Copies values from source to dest
    if (t_argc < 2)
    {
        script_error("copyinto: requires two array arguments\n");
        return;
    }
    
    if (t_argv[0].type != svt_array || t_argv[1].type != svt_array)
    {
        script_error("copyinto: arguments must be arrays\n");
        return;
    }
    
    fs_array_t *src = t_argv[0].value.array;
    fs_array_t *dst = t_argv[1].value.array;
    
    if (!src || !dst)
    {
        script_error("copyinto: invalid array\n");
        return;
    }
    
    // Copy values
    int count = (src->length < dst->length) ? src->length : dst->length;
    for (int i = 0; i < count; i++)
    {
        dst->values[i] = src->values[i];
    }
}

void SF_ArrayElementAt()
{
    // elementat(array, index)
    // Returns element at given index
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 2)
    {
        script_error("elementat: requires array and index\n");
        return;
    }
    
    if (t_argv[0].type != svt_array)
    {
        script_error("elementat: first argument must be array\n");
        return;
    }
    
    fs_array_t *arr = t_argv[0].value.array;
    if (!arr)
    {
        script_error("elementat: invalid array\n");
        return;
    }
    
    int idx = intvalue(t_argv[1]);
    if (idx < 0 || idx >= (int)arr->length)
    {
        script_error("elementat: index out of bounds\n");
        return;
    }
    
    t_return = arr->values[idx];
}

void SF_ArraySetElementAt()
{
    // setelementat(array, index, value)
    // Sets element at given index
    if (t_argc < 3)
    {
        script_error("setelementat: requires array, index and value\n");
        return;
    }
    
    if (t_argv[0].type != svt_array)
    {
        script_error("setelementat: first argument must be array\n");
        return;
    }
    
    fs_array_t *arr = t_argv[0].value.array;
    if (!arr)
    {
        script_error("setelementat: invalid array\n");
        return;
    }
    
    int idx = intvalue(t_argv[1]);
    if (idx < 0 || idx >= (int)arr->length)
    {
        script_error("setelementat: index out of bounds\n");
        return;
    }
    
    arr->values[idx] = t_argv[2];
}

void SF_ArrayLength()
{
    // length(array)
    // Returns length of array
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 1)
    {
        script_error("length: requires array argument\n");
        return;
    }
    
    if (t_argv[0].type != svt_array)
    {
        // For non-arrays, return -1 to indicate error
        t_return.value.i = -1;
        return;
    }
    
    fs_array_t *arr = t_argv[0].value.array;
    if (!arr)
    {
        t_return.value.i = 0;
        return;
    }
    
    t_return.value.i = arr->length;
}

//////////////////////////////////////////////////////////////////////////
// Type conversion functions - ported from legacy_one
//////////////////////////////////////////////////////////////////////////

void SF_MobjValue()
{
    // mobjvalue(mobj)
    // Returns numeric value of mobj (for comparisons)
    t_return.type = svt_fixed;
    
    if (t_argc < 1)
    {
        t_return.value.i = -1;
        return;
    }
    
    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
    {
        t_return.value.i = -1;
        return;
    }
    
    // Return something useful - maybe health or index
    t_return.value.i = mo->health;
}

void SF_StringValue()
{
    // stringvalue(value)
    // Converts value to string
    t_return.type = svt_string;
    t_return.value.s = NULL;
    
    if (t_argc < 1)
    {
        t_return.value.s = "";
        return;
    }
    
    t_return.value.s = stringvalue(t_argv[0]);
}

void SF_IntValue()
{
    // intvalue(value)
    // Converts value to int
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 1)
    {
        return;
    }
    
    t_return.value.i = intvalue(t_argv[0]);
}

void SF_FixedValue()
{
    // fixedvalue(value)
    // Converts value to fixed
    t_return.type = svt_fixed;
    t_return.value.i = 0;
    
    if (t_argc < 1)
    {
        return;
    }
    
    t_return.value.i = fixedvalue(t_argv[0]).value();
}

//////////////////////////////////////////////////////////////////////////
// Additional mobj functions from legacy_one
//////////////////////////////////////////////////////////////////////////

void SF_SpawnMissile()
{
    // spawnmissile(source, target, type)
    t_return.type = svt_actor;
    t_return.value.mobj = NULL;
    
    if (t_argc < 3)
    {
        script_error("spawnmissile: requires source, target, and type\n");
        return;
    }
    
    Actor *source = MobjForSvalue(t_argv[0]);
    Actor *target = MobjForSvalue(t_argv[1]);
    mobjtype_t type = mobjtype_t(intvalue(t_argv[2]));
    
    if (!source || !target)
    {
        script_error("spawnmissile: invalid source or target\n");
        return;
    }
    
    if (type < 0 || type >= NUMMOBJTYPES)
    {
        script_error("spawnmissile: invalid type\n");
        return;
    }
    
    // Spawn missile at source position targeting target
    DActor *missile = current_map->SpawnDActor(source->pos, type);
    if (missile)
    {
        // Calculate angle to target
        missile->yaw = R_PointToAngle2(source->pos, target->pos);
        missile->target = source;
    }
    
    t_return.value.mobj = missile;
}

void SF_RadiusAttack()
{
    // radiusattack(source, target, damage)
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 3)
    {
        script_error("radiusattack: requires source, target, and damage\n");
        return;
    }
    
    Actor *source = MobjForSvalue(t_argv[0]);
    Actor *target = MobjForSvalue(t_argv[1]);
    int damage = intvalue(t_argv[2]);
    
    if (!source || !target)
    {
        return;
    }
    
    // Simple radius attack implementation - check distance
    fixed_t dist = R_PointToDist2(source->pos.x, source->pos.y, target->pos.x, target->pos.y);
    fixed_t radius = 64 * fixed_t::UNIT; // 64 units default radius
    
    if (dist <= radius)
    {
        target->Damage(source, source, damage);
        t_return.value.i = 1;
    }
}

void SF_SetObjPosition()
{
    // setobjposition(mobj, x, y, z)
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 3)
    {
        script_error("setobjposition: requires mobj, x, y and optionally z\n");
        return;
    }
    
    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
    {
        return;
    }
    
    fixed_t x = fixedvalue(t_argv[1]);
    fixed_t y = fixedvalue(t_argv[2]);
    fixed_t z = (t_argc >= 4) ? fixedvalue(t_argv[3]) : mo->pos.z;
    
    // Check if position is valid using TryMove
    if (!mo->TryMove(x, y, true).first)
    {
        t_return.value.i = 0;
        return;
    }
    
    // Set position directly
    mo->pos.x = x;
    mo->pos.y = y;
    mo->pos.z = z;
    mo->subsector = current_map->GetSubsector(x, y);
    t_return.value.i = 1;
}

void SF_TestLocation()
{
    // testlocation(x, y, z) - simplified version using TryMove
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 2)
    {
        script_error("testlocation: requires x, y and optionally z\n");
        return;
    }
    
    fixed_t x = fixedvalue(t_argv[0]);
    fixed_t y = fixedvalue(t_argv[1]);
    
    // Create a minimal actor on stack for testing position validity
    // We'll use TryMove to test if the position is valid
    // This is a simplified test - we create a temp Actor
    Actor tempActor;
    tempActor.pos.x = x;
    tempActor.pos.y = y;
    tempActor.pos.z = (t_argc >= 3) ? fixedvalue(t_argv[2]) : fixed_t(0);
    tempActor.subsector = current_map->GetSubsector(x, y);
    
    if (tempActor.TryMove(x, y, true).first)
    {
        t_return.value.i = 1;
    }
}

void SF_CheckSight()
{
    // checksight(obj1, obj2)
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 2)
    {
        script_error("checksight: requires two objects\n");
        return;
    }
    
    Actor *obj1 = MobjForSvalue(t_argv[0]);
    Actor *obj2 = MobjForSvalue(t_argv[1]);
    
    if (!obj1 || !obj2)
    {
        return;
    }
    
    t_return.value.i = current_map->CheckSight(obj1, obj2) ? 1 : 0;
}

void SF_ClockTic()
{
    // clocktic()
    // Returns the current tic counter
    t_return.type = svt_int;
    t_return.value.i = current_map->maptic;
}

// Exl: Modified by Tox to take a pitch parameter
// lineattack(sourcemobj, damage=0, yaw=source.yaw, range, pitch)
void SF_LineAttack()
{
    t_return.type = svt_actor;
    t_return.value.mobj = NULL;

    if (t_argc < 1 || t_argc > 5)
    {
        script_error("invalid number of arguments\n");
        return;
    }

    Actor *mo = MobjForSvalue(t_argv[0]);
    int damage = (t_argc >= 2) ? intvalue(t_argv[1]) : 0;
    angle_t yaw = (t_argc >= 3) ? intvalue(t_argv[2]) * (ANG45 / 45) : mo->yaw;
    float dist = (t_argc >= 4) ? fixedvalue(t_argv[3]).Float() : 32 * 64;
    float sine;
    if (t_argc >= 5)
    {
        sine = sinf(fixedvalue(t_argv[4]).Float());
    }
    else
    {
        // use autoaim
        if (!mo->AimLineAttack(yaw, dist, sine) && !damage)
            return; // no target to be found, and no damage means no activation effects either
    }

    PuffType = MT_PUFF;
    t_return.value.mobj = mo->LineAttack(yaw, dist, sine, damage, dt_normal);
}

//////////////////////////////////////////////////////////////////////////
// Additional functions from legacy_one - Level/Game (simplified for API compatibility)
//////////////////////////////////////////////////////////////////////////

void SF_Warp()
{
    // warp(mapnum, {x}, {y}, {angle})
    // Warp player to map
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 1)
    {
        script_error("warp: requires map number\n");
        return;
    }
    
    if (trigger_player && trigger_player->pawn)
    {
        if (t_argc >= 3)
        {
            trigger_player->pawn->pos.x = fixedvalue(t_argv[1]);
            trigger_player->pawn->pos.y = fixedvalue(t_argv[2]);
        }
        
        if (t_argc >= 4)
        {
            trigger_player->pawn->yaw = intvalue(t_argv[3]) * (ANG45 / 45);
        }
        
        t_return.value.i = 1;
    }
}

void SF_GameSkill()
{
    t_return.type = svt_int;
    t_return.value.i = game.skill;
}

void SF_PlayerAddFrag()
{
    // playeraddfrag(player1, {player2}) - simplified
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 1)
    {
        script_error("playeraddfrag: requires player argument\n");
        return;
    }
    
    // fragcount not available in current API
    t_return.value.i = 1;
}

void SF_SkinColor()
{
    // skincolor - simplified
    t_return.type = svt_int;
    t_return.value.i = 0;
    script_error("skincolor: not available in this version\n");
}

void SF_PlayerKeysByte()
{
    // playerkeysbyte(player, {newbyte})
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 1)
    {
        script_error("playerkeysbyte: requires player argument\n");
        return;
    }
    
    PlayerPawn *p = GetPawn(t_argv[0]);
    if (!p)
        return;
    
    if (t_argc >= 2)
    {
        p->keycards = intvalue(t_argv[1]);
    }
    
    t_return.value.i = p->keycards;
}

void SF_PlayerArmor()
{
    // playerarmor - simplified (armorpoints is array)
    t_return.type = svt_int;
    t_return.value.i = 0;
    script_error("playerarmor: not available in this version\n");
}

void SF_PlayerWeapon()
{
    // playerweapon - simplified
    t_return.type = svt_int;
    t_return.value.i = 0;
    script_error("playerweapon: not available in this version\n");
}

void SF_PlayerSelectedWeapon()
{
    // playerselectedweapon - simplified
    t_return.type = svt_int;
    t_return.value.i = 0;
    script_error("playerselectedweapon: not available in this version\n");
}

void SF_PlayerPitch()
{
    // playerpitch - simplified
    t_return.type = svt_fixed;
    t_return.value.i = 0;
    script_error("playerpitch: not available in this version\n");
}

void SF_PlayerProperty()
{
    // playerproperty - simplified
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (!trigger_player || !trigger_player->pawn)
        return;
    
    int prop = t_argc >= 1 ? intvalue(t_argv[0]) : 0;
    int val = t_argc >= 2 ? intvalue(t_argv[1]) : 0;
    
    // Property types: 0=health
    switch (prop)
    {
        case 0: // health
            if (t_argc >= 2)
                trigger_player->pawn->health = val;
            t_return.value.i = trigger_player->pawn->health;
            break;
        default:
            break;
    }
}

//////////////////////////////////////////////////////////////////////////
// Additional mobj functions from legacy_one (simplified for API compatibility)
//////////////////////////////////////////////////////////////////////////

void SF_Resurrect()
{
    // resurrect(mobj) - simplified
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 1)
    {
        script_error("resurrect: requires mobj argument\n");
        return;
    }
    
    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
        return;
    
    // Clear MF_CORPSE flag
    if (mo->flags & MF_CORPSE)
    {
        mo->flags &= ~MF_CORPSE;
        // Note: Can't access spawnhealth without proper API
        t_return.value.i = 1;
    }
}

void SF_ObjFlag2()
{
    // objflag2(mobj, flag, {value}) - Extended flag
    t_return.type = svt_int;
    
    if (t_argc < 2)
    {
        script_error("objflag2: requires mobj and flag arguments\n");
        t_return.value.i = 0;
        return;
    }
    
    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
    {
        t_return.value.i = 0;
        return;
    }
    
    int f = intvalue(t_argv[1]);
    
    if (t_argc >= 3)
    {
        int val = intvalue(t_argv[2]);
        if (val)
            mo->eflags |= (1 << f);
        else
            mo->eflags &= ~(1 << f);
    }
    
    t_return.value.i = (mo->eflags & (1 << f)) ? 1 : 0;
}

void SF_ObjEFlag()
{
    // Same as objflag2
    SF_ObjFlag2();
}

void SF_ObjType()
{
    // objtype - simplified (type member may not exist)
    t_return.type = svt_int;
    t_return.value.i = -1;
    script_error("objtype: not available in this version\n");
}

void SF_SetObjProperty()
{
    // setobjproperty(mobj, property, value) - simplified
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 3)
    {
        script_error("setobjproperty: requires mobj, property, and value\n");
        return;
    }
    
    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
        return;
    
    int prop = intvalue(t_argv[1]);
    int val = intvalue(t_argv[2]);
    
    switch (prop)
    {
        case 0: // health
            mo->health = val;
            break;
        case 1: // reaction time
            mo->reactiontime = val;
            break;
        default:
            break;
    }
    
    t_return.value.i = 1;
}

void SF_GetObjProperty()
{
    // getobjproperty({mobj}, property) - simplified
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 2)
    {
        script_error("getobjproperty: requires mobj and property\n");
        return;
    }
    
    Actor *mo = MobjForSvalue(t_argv[0]);
    if (!mo)
        return;
    
    int prop = intvalue(t_argv[1]);
    
    switch (prop)
    {
        case 0: // health
            t_return.value.i = mo->health;
            break;
        case 1: // reaction time
            t_return.value.i = mo->reactiontime;
            break;
        default:
            break;
    }
}

void SF_ObjState()
{
    // objstate - simplified
    t_return.type = svt_int;
    t_return.value.i = 0;
    script_error("objstate: not available in this version\n");
}

void SF_HealObj()
{
    // healobj({mobj}, {health})
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    Actor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
    if (!mo)
        return;
    
    int heal = (t_argc >= 2) ? intvalue(t_argv[1]) : 1;
    
    mo->health += heal;
    // Note: Can't check spawnhealth without proper API
    
    t_return.value.i = mo->health;
}

//////////////////////////////////////////////////////////////////////////
// Mapthing functions from legacy_one
//////////////////////////////////////////////////////////////////////////

void SF_MapthingNumExist()
{
    // mapthingnumexist(thingnum) - returns 1 if thingnum is in range and has a live actor
    t_return.type = svt_int;
    t_return.value.i = 0;

    if (t_argc < 1)
    {
        script_error("mapthingnumexist: requires thing number\n");
        return;
    }

    int thingnum = intvalue(t_argv[0]);

    if (thingnum >= 0 && thingnum < current_map->nummapthings &&
        current_map->mapthings[thingnum].mobj != NULL)
        t_return.value.i = 1;
}

void SF_Mapthings()
{
    // mapthings() - Return array of mapthings
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    // Would need to return array of mapthings - complex
    script_error("mapthings: not fully implemented\n");
}

void SF_PlayDemo()
{
    // playdemo(lumpname)
    if (t_argc < 1)
    {
        script_error("playdemo: requires demo name\n");
        return;
    }
    
    if (t_argv[0].type != svt_string)
    {
        script_error("playdemo: argument must be string\n");
        return;
    }
    
    // Would need demo playback system
    CONS_Printf("playdemo: %s\n", t_argv[0].value.s);
}

void SF_CheckCVar()
{
    // checkcvar(cvarname)
    t_return.type = svt_string;
    t_return.value.s = NULL;
    
    if (t_argc < 1)
    {
        script_error("checkcvar: requires cvar name\n");
        return;
    }
    
    // Would need cvar lookup
    const char *cvarName = stringvalue(t_argv[0]);
    // Simplified: return empty string
    t_return.value.s = "";
}

void SF_SetLineTexture()
{
    // setlinetexture(side, position, texturename)
    // position: 1=top, 2=mid, 3=bottom
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 3)
    {
        script_error("setlinetexture: requires side, position and texture\n");
        return;
    }
    
    // Would need linedef/side access
    script_error("setlinetexture: not fully implemented\n");
}

void SF_SectorEffect()
{
    // sectoreffect(tagnum, effect)
    if (t_argc < 2)
    {
        script_error("sectoreffect: requires tag and effect\n");
        return;
    }
    
    int tagnum = intvalue(t_argv[0]);
    int effect = intvalue(t_argv[1]);
    
    // Various sector effects (light, etc.)
    switch (effect)
    {
        case 1: // Light flicker
        case 2: // Light strobe
        case 3: // Light glow
            // Would need thinker system
            break;
        default:
            break;
    }
}

void SF_SetFade()
{
    // setfade(red, green, blue, alpha)
    t_return.type = svt_int;
    t_return.value.i = 0;
    
    if (t_argc < 4)
    {
        script_error("setfade: requires r, g, b, a\n");
        return;
    }
    
    // Would need global fade setting
    script_error("setfade: not fully implemented\n");
}

void SF_WaitTic()
{
    // waittic(tics)
    if (t_argc < 1)
    {
        script_error("waittic: requires tics\n");
        return;
    }
    
    current_script->save(rover, wt_delay, intvalue(t_argv[0]));
}

//////////////////////////////////////////////////////////////////////////
//
// Init Functions
//

// extern int fov; // r_main.c
int fov;

void FS_init_functions()
{
    // add all the functions
    // add_game_int("consoleplayer", &consoleplayer);
    // add_game_int("displayplayer", &displayplayer);
    add_game_int("fov", &fov);
    add_game_int("zoom", &fov); // SoM: BAKWARDS COMPATABILITY!
    add_game_mobj("trigger", &trigger_obj);

    // important C-emulating stuff
    new_function("break", SF_Break);
    new_function("continue", SF_Continue);
    new_function("return", SF_Return);
    new_function("goto", SF_Goto);
    new_function("include", SF_Include);

    // standard FraggleScript functions
    new_function("print", SF_Print);
    new_function("rnd", SF_Rnd);
    new_function("prnd", SF_PRnd);
    new_function("input", SF_Input); // TODO: document
    new_function("beep", SF_Beep);
    new_function("clock", SF_Clock);
    new_function("wait", SF_Wait);
    new_function("tagwait", SF_TagWait);
    new_function("scriptwait", SF_ScriptWait);
    new_function("startscript", SF_StartScript);
    new_function("scriptrunning", SF_ScriptRunning);

    // doom stuff
    new_function("startskill", SF_StartSkill);
    new_function("exitlevel", SF_ExitLevel);
    new_function("tip", SF_Tip);
    new_function("timedtip", SF_TimedTip);
    new_function("message", SF_Message);
    new_function("gamemode", SF_GameMode); // SoM Request SSNTails 06-13-2002

    // player stuff
    new_function("playermsg", SF_PlayerMsg);
    new_function("playertip", SF_PlayerTip);
    new_function("playeringame", SF_PlayerInGame);
    new_function("playername", SF_PlayerName);
    new_function("playerobj", SF_PlayerObj);
    new_function("isobjplayer", SF_MobjIsPlayer);
    new_function("isplayerobj", SF_MobjIsPlayer); // backward compatibility
    new_function("playerkeys", SF_PlayerKeys);
    new_function("playerammo", SF_PlayerAmmo);
    new_function("maxplayerammo", SF_MaxPlayerAmmo);

    // mobj stuff
    new_function("spawn", SF_Spawn);
    new_function("spawnexplosion", SF_SpawnExplosion);
    new_function("kill", SF_KillObj);
    new_function("removeobj", SF_RemoveObj);
    new_function("objx", SF_ObjX);
    new_function("objy", SF_ObjY);
    new_function("objz", SF_ObjZ);
    new_function("teleport", SF_Teleport);
    new_function("silentteleport", SF_SilentTeleport);
    new_function("damageobj", SF_DamageObj);
    new_function("player", SF_Player);
    new_function("objsector", SF_ObjSector);
    new_function("objflag", SF_ObjFlag);
    new_function("pushobj", SF_PushThing);
    new_function("pushthing", SF_PushThing); // backward compatibility
    new_function("objangle", SF_ObjAngle);
    new_function("objhealth", SF_ObjHealth);
    new_function("objdead", SF_ObjDead);
    new_function("objreactiontime", SF_ReactionTime);
    new_function("reactiontime", SF_ReactionTime); // backward compatibility
    new_function("objtarget", SF_MobjTarget);
    new_function("objmomx", SF_MobjMomx);
    new_function("objmomy", SF_MobjMomy);
    new_function("objmomz", SF_MobjMomz);

    new_function("lineattack", SF_LineAttack);

    // sector stuff
    new_function("floorheight", SF_FloorHeight);
    new_function("floortext", SF_FloorTexture);
    new_function("floortexture", SF_FloorTexture); // b comp.
    new_function("movefloor", SF_MoveFloor);
    new_function("ceilheight", SF_CeilingHeight);
    new_function("ceilingheight", SF_CeilingHeight); // b comp.
    new_function("moveceil", SF_MoveCeiling);
    new_function("moveceiling", SF_MoveCeiling); // b comp.
    new_function("ceiltext", SF_CeilingTexture);
    new_function("ceilingtexture", SF_CeilingTexture); // b comp.
    new_function("lightlevel", SF_LightLevel);
    new_function("fadelight", SF_FadeLight);
    new_function("colormap", SF_SectorColormap);

    // cameras!
    new_function("setcamera", SF_SetCamera);
    new_function("clearcamera", SF_ClearCamera);
    new_function("movecamera", SF_MoveCamera);

    // trig functions
    new_function("pointtoangle", SF_PointToAngle);
    new_function("pointtodist", SF_PointToDist);

    // sound functions
    new_function("startsound", SF_StartSound);
    new_function("startsectorsound", SF_StartSectorSound);
    new_function("startambientsound", SF_StartAmbientSound);
    new_function("startambiantsound", SF_AmbientSound); // b comp., misspelled...
    new_function("ambientsound", SF_AmbientSound);      // b comp.
    new_function("changemusic", SF_ChangeMusic);

    // hubs!
    new_function("changehublevel", SF_ChangeHubLevel); // TODO: document

    // doors
    new_function("opendoor", SF_OpenDoor);
    new_function("closedoor", SF_CloseDoor);

    new_function("runcommand", SF_RunCommand);
    new_function("linetrigger", SF_LineTrigger);
    new_function("lineflag", SF_LineFlag); // TODO: document

    new_function("max", SF_Max);
    new_function("min", SF_Min);
    new_function("abs", SF_Abs);

    // Hurdler: new math functions
    new_function("sin", SF_Sin);
    new_function("asin", SF_ASin);
    new_function("cos", SF_Cos);
    new_function("acos", SF_ACos);
    new_function("tan", SF_Tan);
    new_function("atan", SF_ATan);
    new_function("exp", SF_Exp);
    new_function("log", SF_Log);
    new_function("sqrt", SF_Sqrt);
    new_function("floor", SF_Floor);
    new_function("pow", SF_Pow);

    // HU Graphics
    new_function("newhupic", SF_NewHUPic);
    new_function("createpic", SF_NewHUPic);
    new_function("deletehupic", SF_DeleteHUPic);
    new_function("modifyhupic", SF_ModifyHUPic);
    new_function("modifypic", SF_ModifyHUPic);
    new_function("sethupicdisplay", SF_SetHUPicDisplay);
    new_function("setpicvisible", SF_SetHUPicDisplay);

    // Hurdler's stuff :)
    new_function("setcorona", SF_SetCorona);
    
    // Array functions - from legacy_one
    new_function("newarray", SF_NewArray);
    new_function("newemptyarray", SF_NewEmptyArray);
    new_function("copyinto", SF_ArrayCopyInto);
    new_function("elementat", SF_ArrayElementAt);
    new_function("setelementat", SF_ArraySetElementAt);
    new_function("length", SF_ArrayLength);
    
    // Type conversion functions - from legacy_one
    new_function("mobjvalue", SF_MobjValue);
    new_function("stringvalue", SF_StringValue);
    new_function("intvalue", SF_IntValue);
    new_function("fixedvalue", SF_FixedValue);
    
    // Additional mobj functions - from legacy_one
    new_function("spawnmissile", SF_SpawnMissile);
    new_function("radiusattack", SF_RadiusAttack);
    new_function("setobjposition", SF_SetObjPosition);
    new_function("testlocation", SF_TestLocation);
    new_function("checksight", SF_CheckSight);
    new_function("clocktic", SF_ClockTic);
    
    // Level/Game functions - from legacy_one
    new_function("warp", SF_Warp);
    new_function("gameskill", SF_GameSkill);
    new_function("playeraddfrag", SF_PlayerAddFrag);
    new_function("skincolor", SF_SkinColor);
    new_function("playerkeysbyte", SF_PlayerKeysByte);
    new_function("playerarmor", SF_PlayerArmor);
    new_function("playerweapon", SF_PlayerWeapon);
    new_function("playerselectedweapon", SF_PlayerSelectedWeapon);
    new_function("playerpitch", SF_PlayerPitch);
    new_function("playerproperty", SF_PlayerProperty);
    
    // Additional mobj functions - from legacy_one
    new_function("resurrect", SF_Resurrect);
    new_function("objflag2", SF_ObjFlag2);
    new_function("objeflag", SF_ObjEFlag);
    new_function("objtype", SF_ObjType);
    new_function("setobjproperty", SF_SetObjProperty);
    new_function("getobjproperty", SF_GetObjProperty);
    new_function("objstate", SF_ObjState);
    new_function("healobj", SF_HealObj);
    
    // Mapthing and misc functions - from legacy_one
    new_function("mapthingnumexist", SF_MapthingNumExist);
    new_function("mapthings", SF_Mapthings);
    new_function("playdemo", SF_PlayDemo);
    new_function("checkcvar", SF_CheckCVar);
    new_function("setlinetexture", SF_SetLineTexture);
    new_function("sectoreffect", SF_SectorEffect);
    new_function("setfade", SF_SetFade);
    new_function("waittic", SF_WaitTic);
}
