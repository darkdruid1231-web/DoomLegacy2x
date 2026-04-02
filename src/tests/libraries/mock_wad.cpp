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
/// \brief Mock implementation of IWadRepository for unit testing.
///
/// Usage:
///   MockWadRepository mockWad;
///   // Set up expectations:
///   EXPECT_CALL(mockWad, findLump(::testing::StrEq("PLAYPAL"), ::testing::_))
///       .WillOnce(::testing::Return(0x00010005));  // file 1, lump 5
///   // Use in component under test:
///   SomeComponent component(&mockWad);

#include "interfaces/i_wad.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Prevent SDL_main renaming
#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

class MockWadRepository : public IWadRepository {
public:
    // Google Mock method declarations
    MOCK_METHOD(int, findLump, (const char* name), (const override));
    MOCK_METHOD(bool, readLump, (int lumpnum, void* dest), (const override));
    MOCK_METHOD(int, getLumpSize, (int lumpnum), (const override));
    MOCK_METHOD(bool, lumpExists, (const char* name), (const override));
};
