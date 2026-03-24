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
/// \brief Mock implementation of IGameStateProvider for unit testing.

#include "interfaces/i_game.h"
#include "game/g_player.h"   // for PlayerInfo
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Prevent SDL_main renaming
#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

class MockGameStateProvider : public IGameStateProvider {
public:
    // Google Mock method declarations
    MOCK_METHOD(int, getGameState, (), (const override));
    MOCK_METHOD(unsigned int, getGameTic, (), (const override));
    MOCK_METHOD(bool, isPaused, (), (const override));
    MOCK_METHOD(bool, isNetworkGame, (), (const override));
    MOCK_METHOD(bool, isMultiplayer, (), (const override));
    MOCK_METHOD(Map*, getCurrentMap, (), (const override));
    MOCK_METHOD(int, getCurrentMapLump, (), (const override));
    MOCK_METHOD(PlayerInfo*, findPlayer, (int playernum), (const override));
    MOCK_METHOD(int, getPlayerCount, (), (const override));
    MOCK_METHOD(bool, isPlayerAlive, (const PlayerInfo* p), (const override));
    MOCK_METHOD(int, getPlayerHealth, (const PlayerInfo* p), (const override));
    MOCK_METHOD(void, getPlayerPosition, (const PlayerInfo* p, int& x, int& y, int& z), (const override));
    MOCK_METHOD(PlayerInfo*, getLocalPlayer, (int index), (const override));
    MOCK_METHOD(bool, pollEvent, (event_t& ev), (override));
};
