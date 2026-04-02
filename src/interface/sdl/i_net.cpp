// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_net.cpp 308 2006-02-11 16:54:43Z jussip $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
//
//
// Description:
//      network interface using ENet
//
//-----------------------------------------------------------------------------

#include <errno.h>

#include "doomdef.h"

#include "d_event.h"
#include "i_system.h"
#include "m_argv.h"

#include "i_net.h"

#include "z_zone.h"

#include <enet/enet.h>

//
// I_InitNetwork
// Initializes ENet networking
//
bool I_InitNetwork(void)
{
    // Check for legacy -net option
    if (M_CheckParm("-net"))
    {
        I_Error("-net not supported, use -server and -connect\n"
                "see docs for more\n");
    }

    // Initialize ENet library
    if (enet_initialize() != 0)
    {
        CONS_Printf("Failed to initialize ENet\n");
        return false;
    }

    CONS_Printf("ENet networking initialized\n");

    // TODO: Process -server and -connect command line options here
    // This would typically be done in d_main.c or similar

    return true;
}
