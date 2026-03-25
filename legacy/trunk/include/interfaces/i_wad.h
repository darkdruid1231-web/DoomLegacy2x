//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief WAD repository interface for dependency injection in tests.
///
/// This interface abstracts the FileCache functionality to enable mock-based
/// unit testing. Production code uses WadRepositoryAdapter which delegates
/// to the global FileCache instance.
///
/// Usage in production:
///   IWadRepository* wad = GetGlobalWadRepository(); // returns adapter
///
/// Usage in tests:
///   MockWadRepository mock;
///   ComponentUnderTest c(mock);  // injects mock
///
/// To add a new method to this interface:
///   1. Add the pure virtual method to this class
///   2. Implement in WadRepositoryAdapter (util/w_wad.cpp)
///   3. Override in MockWadRepository (tests/libraries/mock_wad.cpp)
///
/// NOTE: This is a minimal interface focused on testable components.
//-----------------------------------------------------------------------------

#ifndef INCLUDED_INTERFACES_I_WAD_H
#define INCLUDED_INTERFACES_I_WAD_H

#include <string>

class IWadRepository {
public:
    virtual ~IWadRepository() = default;

    /// Find a lump by name.
    /// \param name 8-char uppercase lump name (e.g., "PLAYPAL", "T_START")
    /// \return Lump number (filenum<<16 | lumpindex), or -1 if not found
    virtual int findLump(const char* name) const = 0;

    /// Read entire lump contents into buffer.
    /// \param lumpnum Lump number
    /// \param dest Destination buffer (must be sized appropriately)
    /// \return true on success, false on failure
    virtual bool readLump(int lumpnum, void* dest) const = 0;

    /// Get lump size in bytes.
    /// \param lumpnum Lump number
    /// \return Size in bytes, or -1 if lump not found
    virtual int getLumpSize(int lumpnum) const = 0;

    /// Check if a lump exists (findLump returns != -1).
    virtual bool lumpExists(const char* name) const = 0;
};

#endif // INCLUDED_INTERFACES_I_WAD_H
