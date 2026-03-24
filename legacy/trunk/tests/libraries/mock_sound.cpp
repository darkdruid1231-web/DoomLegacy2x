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
/// \brief Mock implementation of ISoundSystem for unit testing.
///
/// Usage:
///   MockSoundSystem mockSound;
///   // Set up expectations:
///   EXPECT_CALL(mockSound, startSound(::testing::_, ::testing::Eq(sfx_pain), ::testing::_))
///       .WillOnce(::testing::Return(2));  // channel 2
///   // Use in component under test:
///   MonsterAI monster(&mockSound);

#include "interfaces/i_sound.h"
#include "g_actor.h"      // For mobj_t definition
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Prevent SDL_main renaming
#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

class MockSoundSystem : public ISoundSystem {
public:
    // Google Mock method declarations
    MOCK_METHOD(int, startSound, (mobj_t* mobj, int sfx_id, float vol), (override));
    MOCK_METHOD(void, stopSound, (mobj_t* mobj), (override));
    MOCK_METHOD(void, setListenerPosition, (float x, float y, float z), (override));
    MOCK_METHOD(bool, isChannelPlaying, (int channel), (const override));
    MOCK_METHOD(void, setChannelVolume, (int channel, float vol), (override));
    MOCK_METHOD(int, getActiveChannelCount, (), (const override));
    MOCK_METHOD(void, precacheSound, (int sfx_id), (override));
};
