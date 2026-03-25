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
/// \brief Google Test tests for HUDComponent using MockGameStateProvider.

#include "interfaces/i_game.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::Pointee;

// Local mock matching IGameStateProvider interface
class MockGameStateProviderLocal : public IGameStateProvider {
public:
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

// HUD component that reads player state for display
class HUDComponent {
public:
    explicit HUDComponent(IGameStateProvider* game) : game_(game) {}

    bool shouldShowHUD() const {
        return game_->getGameState() == 1 && !game_->isPaused();  // GS_LEVEL
    }

    int getDisplayHealth() const {
        PlayerInfo* p = game_->getLocalPlayer(0);
        if (!p || !game_->isPlayerAlive(p))
            return 0;
        return game_->getPlayerHealth(p);
    }

    bool shouldDrawAmmo() const {
        return shouldShowHUD() && game_->getPlayerCount() > 0;
    }

    bool isNetworkGame() const {
        return game_->isNetworkGame();
    }

private:
    IGameStateProvider* game_;
};

// Tests

TEST(HUDComponent, showsHUDDuringGameplay)
{
    NiceMock<MockGameStateProviderLocal> mock;

    ON_CALL(mock, getGameState()).WillByDefault(Return(1));  // GS_LEVEL
    ON_CALL(mock, isPaused()).WillByDefault(Return(false));

    HUDComponent hud(&mock);
    EXPECT_TRUE(hud.shouldShowHUD());
}

TEST(HUDComponent, hidesHUDWhenPaused)
{
    NiceMock<MockGameStateProviderLocal> mock;

    ON_CALL(mock, getGameState()).WillByDefault(Return(1));
    ON_CALL(mock, isPaused()).WillByDefault(Return(true));

    HUDComponent hud(&mock);
    EXPECT_FALSE(hud.shouldShowHUD());
}

TEST(HUDComponent, hidesHUDOutsideLevel)
{
    NiceMock<MockGameStateProviderLocal> mock;

    ON_CALL(mock, getGameState()).WillByDefault(Return(0));  // GS_INTRO
    ON_CALL(mock, isPaused()).WillByDefault(Return(false));

    HUDComponent hud(&mock);
    EXPECT_FALSE(hud.shouldShowHUD());
}

TEST(HUDComponent, displaysZeroHealthWhenNoPlayer)
{
    NiceMock<MockGameStateProviderLocal> mock;

    ON_CALL(mock, getGameState()).WillByDefault(Return(1));
    ON_CALL(mock, isPaused()).WillByDefault(Return(false));
    ON_CALL(mock, getLocalPlayer(0)).WillByDefault(Return(nullptr));

    HUDComponent hud(&mock);
    EXPECT_EQ(hud.getDisplayHealth(), 0);
}

TEST(HUDComponent, hidesAmmoWhenNoPlayers)
{
    NiceMock<MockGameStateProviderLocal> mock;

    ON_CALL(mock, getGameState()).WillByDefault(Return(1));
    ON_CALL(mock, isPaused()).WillByDefault(Return(false));
    ON_CALL(mock, getPlayerCount()).WillByDefault(Return(0));

    HUDComponent hud(&mock);
    EXPECT_FALSE(hud.shouldDrawAmmo());
}

TEST(HUDComponent, detectsNetworkGame)
{
    NiceMock<MockGameStateProviderLocal> mock;

    ON_CALL(mock, isNetworkGame()).WillByDefault(Return(true));

    HUDComponent hud(&mock);
    EXPECT_TRUE(hud.isNetworkGame());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
