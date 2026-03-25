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
/// \brief Mock implementation of ISoundPlayer for unit testing.

#include "interfaces/i_sound_player.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

class MockSoundPlayer : public ISoundPlayer {
public:
    MOCK_METHOD(bool, startMusic, (const char* lumpname, bool loop), (override));
    MOCK_METHOD(void, stopMusic, (), (override));
    MOCK_METHOD(void, pauseMusic, (), (override));
    MOCK_METHOD(void, resumeMusic, (), (override));
    MOCK_METHOD(const char*, getCurrentMusic, (), (const override));
    MOCK_METHOD(void, setMusicVolume, (int volume), (override));
    MOCK_METHOD(void, updateCD, (), (override));
    MOCK_METHOD(bool, startMusicByEnum, (int musenum, bool loop), (override));
};
