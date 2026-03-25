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
/// \brief Simple WAD lump reader using IWadRepository interface.
///
/// This class demonstrates dependency injection with IWadRepository.
/// Production code uses WadRepositoryAdapter (from w_wad.cpp).
/// Tests use MockWadRepository (from tests/libraries/mock_wad.cpp).
///
/// Usage in production:
///   WadLumpReader reader(GetGlobalWadRepository());
///   std::vector<byte> data = reader.readLump("PLAYPAL");
///
/// Usage in tests:
///   MockWadRepository mock;
///   EXPECT_CALL(mock, findLump(StrEq("PLAYPAL")))...
///   WadLumpReader reader(&mock);

#ifndef INCLUDED_WAD_LUMP_READER_H
#define INCLUDED_WAD_LUMP_READER_H

#include "interfaces/i_wad.h"
#include <vector>
#include <cstdint>

/// \brief Simple WAD lump reader utility.
///
/// Reads lump data into a std::vector using an IWadRepository.
/// This demonstrates the DI pattern: the reader holds an IWadRepository*
/// which can be either a real adapter (production) or a mock (tests).
class WadLumpReader {
public:
    /// Construct with a WadRepository instance.
    /// The reader does NOT take ownership - caller manages lifetime.
    explicit WadLumpReader(IWadRepository* wad) : wad_(wad) {}

    /// Read a lump by name into a vector.
    /// \param name Lump name (e.g., "PLAYPAL")
    /// \param[out] dest Vector to fill with lump data
    /// \return true on success, false if lump not found or read failed
    bool readIntoVector(const char* name, std::vector<uint8_t>& dest) const {
        int lump = wad_->findLump(name);
        if (lump < 0) {
            return false;
        }
        int size = wad_->getLumpSize(lump);
        if (size <= 0) {
            return false;
        }
        dest.resize(size);
        return wad_->readLump(lump, dest.data());
    }

    /// Check if a lump exists.
    bool lumpExists(const char* name) const {
        return wad_->lumpExists(name);
    }

    /// Get lump size without reading data.
    /// \return lump size in bytes, or -1 if not found
    int getLumpSize(const char* name) const {
        int lump = wad_->findLump(name);
        if (lump < 0) {
            return -1;
        }
        return wad_->getLumpSize(lump);
    }

private:
    IWadRepository* wad_;
};

#endif // INCLUDED_WAD_LUMP_READER_H
