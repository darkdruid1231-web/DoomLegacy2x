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
/// \brief Google Test tests for WadLumpReader using MockWadRepository.
///
/// This demonstrates the full DI pattern:
///   WadLumpReader (component) -> IWadRepository (interface) -> MockWadRepository (test)

#include "wad_lump_reader.h"
#include "interfaces/i_wad.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

// MockWadRepository is compiled in MockWad static library
// (declared here since there's no public header for it)
class MockWadRepository : public IWadRepository {
public:
    MOCK_METHOD(int, findLump, (const char* name), (const override));
    MOCK_METHOD(bool, readLump, (int lumpnum, void* dest), (const override));
    MOCK_METHOD(int, getLumpSize, (int lumpnum), (const override));
    MOCK_METHOD(bool, lumpExists, (const char* name), (const override));
};

//============================================================================
// WadLumpReader Tests
//============================================================================

TEST(WadLumpReader, readIntoVectorSuccess)
{
    MockWadRepository mock;
    WadLumpReader reader(&mock);

    // Sample texture data (simplified - 256 bytes of palette)
    std::vector<uint8_t> expectedData(256);
    for (int i = 0; i < 256; i++) {
        expectedData[i] = static_cast<uint8_t>(i);
    }

    // Set up mock expectations
    EXPECT_CALL(mock, findLump(StrEq("PLAYPAL")))
        .WillOnce(Return(7));  // lump 7
    EXPECT_CALL(mock, getLumpSize(7))
        .WillOnce(Return(256));
    EXPECT_CALL(mock, readLump(7, _))
        .WillOnce([&](int lump, void* dest) {
            std::memcpy(dest, expectedData.data(), 256);
            return true;
        });

    // Execute
    std::vector<uint8_t> result;
    bool success = reader.readIntoVector("PLAYPAL", result);

    // Verify
    EXPECT_TRUE(success);
    EXPECT_EQ(result.size(), 256u);
    EXPECT_EQ(result[0], 0);
    EXPECT_EQ(result[128], 128);
}

TEST(WadLumpReader, readIntoVectorNotFound)
{
    MockWadRepository mock;
    WadLumpReader reader(&mock);

    EXPECT_CALL(mock, findLump(StrEq("NOTEXIST")))
        .WillOnce(Return(-1));

    std::vector<uint8_t> result;
    bool success = reader.readIntoVector("NOTEXIST", result);

    EXPECT_FALSE(success);
    EXPECT_TRUE(result.empty());
}

TEST(WadLumpReader, readIntoVectorZeroSize)
{
    MockWadRepository mock;
    WadLumpReader reader(&mock);

    EXPECT_CALL(mock, findLump(StrEq("EMPTY")))
        .WillOnce(Return(3));
    EXPECT_CALL(mock, getLumpSize(3))
        .WillOnce(Return(0));

    std::vector<uint8_t> result;
    bool success = reader.readIntoVector("EMPTY", result);

    EXPECT_FALSE(success);
}

TEST(WadLumpReader, lumpExistsTrue)
{
    MockWadRepository mock;
    WadLumpReader reader(&mock);

    EXPECT_CALL(mock, lumpExists(StrEq("PLAYPAL")))
        .WillOnce(Return(true));

    EXPECT_TRUE(reader.lumpExists("PLAYPAL"));
}

TEST(WadLumpReader, lumpExistsFalse)
{
    MockWadRepository mock;
    WadLumpReader reader(&mock);

    EXPECT_CALL(mock, lumpExists(StrEq("NOTEXIST")))
        .WillOnce(Return(false));

    EXPECT_FALSE(reader.lumpExists("NOTEXIST"));
}

TEST(WadLumpReader, getLumpSizeSuccess)
{
    MockWadRepository mock;
    WadLumpReader reader(&mock);

    EXPECT_CALL(mock, findLump(StrEq("PLAYPAL")))
        .WillOnce(Return(5));
    EXPECT_CALL(mock, getLumpSize(5))
        .WillOnce(Return(768));  // 256 * 3 for RGB

    EXPECT_EQ(reader.getLumpSize("PLAYPAL"), 768);
}

TEST(WadLumpReader, getLumpSizeNotFound)
{
    MockWadRepository mock;
    WadLumpReader reader(&mock);

    EXPECT_CALL(mock, findLump(StrEq("NOTEXIST")))
        .WillOnce(Return(-1));

    EXPECT_EQ(reader.getLumpSize("NOTEXIST"), -1);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
