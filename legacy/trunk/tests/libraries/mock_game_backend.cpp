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
/// \brief Mock implementation of IGameBackend for unit testing.

#include "interfaces/i_game_backend.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

class MockGameBackend : public IGameBackend {
public:
    MOCK_METHOD(void, runGameLoop, (), (override));
    MOCK_METHOD(void, advanceTics, (int elapsed), (override));
    MOCK_METHOD(void, setGameState, (int state), (override));
    MOCK_METHOD(void, startIntro, (), (override));
    MOCK_METHOD(void, advanceIntro, (), (override));
    MOCK_METHOD(void, pauseGame, (bool on), (override));
    MOCK_METHOD(void, updateDisplay, (), (override));
    MOCK_METHOD(void, drawFrame, (), (override));
    MOCK_METHOD(bool, spawnServer, (bool force_mapinfo), (override));
    MOCK_METHOD(bool, isPlaying, (), (override));
    MOCK_METHOD(void, quit, (), (override));
};
