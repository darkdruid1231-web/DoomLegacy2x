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
/// \brief Google Test version of WAD lump lookup tests.
///
/// This test demonstrates the Google Test/Mock infrastructure by testing
/// the IWadRepository interface with MockWadRepository.
///
/// Key patterns demonstrated:
/// - Using Google Test assertions (EXPECT_EQ, EXPECT_TRUE, etc.)
/// - Setting up mock expectations with Google Mock (EXPECT_CALL)
/// - Testing interface contracts without full system dependencies

#include "interfaces/i_wad.h"
#include "functors.h"           // For less_cstring
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

// Import the mock class from the test library
// Note: In a real project with better integration, this would come from a header.
// For now we redeclare it here since the mock is in the same test library.
class MockWadRepository : public IWadRepository {
public:
    MOCK_METHOD(int, findLump, (const char* name), (const override));
    MOCK_METHOD(bool, readLump, (int lumpnum, void* dest), (const override));
    MOCK_METHOD(int, getLumpSize, (int lumpnum), (const override));
    MOCK_METHOD(bool, lumpExists, (const char* name), (const override));
};

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

//============================================================================
// Tests for MockWadRepository expectations
//============================================================================

TEST(MockWadRepository, findLumpReturnsCorrectEncoding)
{
    MockWadRepository mock;

    // Set up expectation: findLump("PLAYPAL") returns file 1, lump 5
    EXPECT_CALL(mock, findLump(StrEq("PLAYPAL")))
        .WillOnce(Return((1 << 16) | 5));

    int result = mock.findLump("PLAYPAL");
    EXPECT_EQ(result, (1 << 16) | 5);
}

TEST(MockWadRepository, lumpExistsReturnsTrueWhenPresent)
{
    MockWadRepository mock;

    // Set up: PLAYPAL exists
    EXPECT_CALL(mock, lumpExists(StrEq("PLAYPAL")))
        .WillOnce(Return(true));

    bool exists = mock.lumpExists("PLAYPAL");
    EXPECT_TRUE(exists);
}

TEST(MockWadRepository, lumpExistsReturnsFalseWhenNotFound)
{
    MockWadRepository mock;

    // Set up: T_NOTFOUND doesn't exist
    EXPECT_CALL(mock, lumpExists(StrEq("T_NOTFOUND")))
        .WillOnce(Return(false));

    bool exists = mock.lumpExists("T_NOTFOUND");
    EXPECT_FALSE(exists);
}

//============================================================================
// Tests for less_cstring (the actual comparison functor)
// These tests don't need mocking - they test pure logic
//============================================================================

TEST(LessCString, caseSensitiveComparison)
{
    less_cstring cmp;

    // Same strings are not less than each other
    EXPECT_FALSE(cmp("PLAYPAL", "PLAYPAL"));
    EXPECT_FALSE(cmp("playpal", "playpal"));

    // In ASCII: uppercase (65-90) < lowercase (97-122)
    // So "PLAYPAL" < "playpal" because 'P' < 'p'
    EXPECT_TRUE(cmp("PLAYPAL", "playpal"));
    EXPECT_FALSE(cmp("playpal", "PLAYPAL"));
}

TEST(LessCString, firstCharDifference)
{
    less_cstring cmp;

    EXPECT_TRUE(cmp("ANIMATED", "PLAYPAL"));   // 'A' < 'P'
    EXPECT_FALSE(cmp("PLAYPAL", "ANIMATED"));

    EXPECT_TRUE(cmp("COLORMAP", "ENDOOM"));     // 'C' < 'E'
    EXPECT_FALSE(cmp("ENDOOM", "COLORMAP"));
}

TEST(LessCString, sortingOfCommonWadNames)
{
    less_cstring cmp;

    // Capital letters (65-90) come before lowercase (97-122) in ASCII
    // Common DOOM lump names should be in sorted order
    const char* names[] = {"ANIMATED", "COLORMAP", "ENDOOM", "F_START", "PLAYPAL", "T_START"};

    for (int i = 1; i < 6; i++)
    {
        EXPECT_FALSE(cmp(names[i], names[i - 1]))
            << names[i] << " should not be less than " << names[i - 1];
    }
}

//============================================================================
// Tests demonstrating mock-based integration testing
//============================================================================

// Example: A hypothetical component that depends on IWadRepository
class TextureLoader {
public:
    explicit TextureLoader(IWadRepository* wad) : wad_(wad) {}

    // Load a texture from a WAD lump
    bool loadTexture(const char* name, void* dest, int maxSize) {
        int lump = wad_->findLump(name);
        if (lump < 0) {
            return false;
        }
        int size = wad_->getLumpSize(lump);
        if (size > maxSize) {
            return false;
        }
        return wad_->readLump(lump, dest);
    }

private:
    IWadRepository* wad_;
};

TEST(TextureLoader, loadsExistingTexture)
{
    MockWadRepository mock;

    const int kTextureSize = 1024;
    char buffer[kTextureSize];

    // Set up expectations for a successful load
    EXPECT_CALL(mock, findLump(StrEq("WALL001")))
        .WillOnce(Return(7));  // lump 7 in file 0
    EXPECT_CALL(mock, getLumpSize(7))
        .WillOnce(Return(512));
    EXPECT_CALL(mock, readLump(7, _))
        .WillOnce(Return(true));

    TextureLoader loader(&mock);
    bool result = loader.loadTexture("WALL001", buffer, kTextureSize);

    EXPECT_TRUE(result);
}

TEST(TextureLoader, failsForNonexistentTexture)
{
    MockWadRepository mock;

    char buffer[1024];

    // Set up: texture not found
    EXPECT_CALL(mock, findLump(StrEq("NOTEXIST")))
        .WillOnce(Return(-1));

    TextureLoader loader(&mock);
    bool result = loader.loadTexture("NOTEXIST", buffer, 1024);

    EXPECT_FALSE(result);
}

TEST(TextureLoader, failsWhenTextureTooLarge)
{
    MockWadRepository mock;

    char buffer[256];  // max size is 256

    EXPECT_CALL(mock, findLump(StrEq("HUGETEX")))
        .WillOnce(Return(3));
    EXPECT_CALL(mock, getLumpSize(3))
        .WillOnce(Return(1024));  // Too big for our buffer

    TextureLoader loader(&mock);
    bool result = loader.loadTexture("HUGETEX", buffer, 256);

    EXPECT_FALSE(result);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
