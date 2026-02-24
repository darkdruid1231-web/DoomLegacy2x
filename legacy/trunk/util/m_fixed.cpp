// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: m_fixed.cpp 367 2006-08-10 16:26:53Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
/// \brief Fixed point math.

#include "m_fixed.h"
#include "i_system.h"
#ifdef TNL_STUB_BUILD
#include "tnl/tnlBitStream.h"
#endif

/// smallest possible increment
fixed_t fixed_epsilon(1 / float(fixed_t::UNIT));

/// Serialization using ISerializer abstraction
void fixed_t::Pack(DoomLegacy::ISerializer &s)
{
    // TODO: save some bandwidth
    s.write(static_cast<uint32_t>(val));
}

void fixed_t::Pack(BitStream *s)
{
    s->writeInt(val, 32);
}

/// Deserialization using ISerializer abstraction
void fixed_t::Unpack(DoomLegacy::ISerializer &s)
{
    val = s.readInt32();
}

void fixed_t::Unpack(BitStream *s)
{
    val = s->readInt(32);
}
