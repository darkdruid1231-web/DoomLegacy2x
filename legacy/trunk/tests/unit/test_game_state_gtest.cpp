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
/// \brief Google Test tests for IGameStateProvider using MockGameStateProvider.
///
/// This demonstrates the full DI pattern:
///   GameComponent (uses IGameStateProvider) -> MockGameStateProvider (test)

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

// Forward declarations for test fixtures
class MockGameStateProvider : public IGameStateProvider {
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

//============================================================================
// Example component that depends on IGameStateProvider
//============================================================================

class GameComponent {
public:
    explicit GameComponent(IGameStateProvider* game) : game_(game) {}

    /// Check if game is in a playable state
    /// \param levelState the expected game state value for GS_LEVEL (typically 1)
    bool isPlayable() const {
        return game_->getGameState() == 1 && !game_->isPaused();
    }

    /// Get the current game tic
    unsigned int gameTime() const {
        return game_->getGameTic();
    }

    /// Check if we should update AI (not paused, in level)
    bool shouldUpdateAI() const {
        return game_->getGameState() == 1 && !game_->isPaused();  // GS_LEVEL = 1
    }

    /// Check if in network game
    bool isNetworkPlay() const {
        return game_->isNetworkGame();
    }

private:
    IGameStateProvider* game_;
};

//============================================================================
// Tests
//============================================================================

TEST(GameStateProvider, gameInPlayableState)
{
    NiceMock<MockGameStateProvider> mock;

    // Set up: game is in level (1) and not paused
    ON_CALL(mock, getGameState()).WillByDefault(Return(1));  // GS_LEVEL
    ON_CALL(mock, isPaused()).WillByDefault(Return(false));

    GameComponent component(&mock);

    EXPECT_TRUE(component.isPlayable());
    EXPECT_TRUE(component.shouldUpdateAI());
    EXPECT_FALSE(component.isNetworkPlay());
}

TEST(GameStateProvider, gamePaused)
{
    NiceMock<MockGameStateProvider> mock;

    ON_CALL(mock, getGameState()).WillByDefault(Return(1));  // GS_LEVEL
    ON_CALL(mock, isPaused()).WillByDefault(Return(true));

    GameComponent component(&mock);

    EXPECT_FALSE(component.isPlayable());
    EXPECT_FALSE(component.shouldUpdateAI());
}

TEST(GameStateProvider, gameInIntro)
{
    NiceMock<MockGameStateProvider> mock;

    ON_CALL(mock, getGameState()).WillByDefault(Return(0));  // GS_INTRO
    ON_CALL(mock, isPaused()).WillByDefault(Return(false));

    GameComponent component(&mock);

    EXPECT_FALSE(component.isPlayable());
    EXPECT_FALSE(component.shouldUpdateAI());
}

TEST(GameStateProvider, gameTicCounter)
{
    NiceMock<MockGameStateProvider> mock;

    ON_CALL(mock, getGameTic()).WillByDefault(Return(12345));

    GameComponent component(&mock);

    EXPECT_EQ(component.gameTime(), 12345u);
}

TEST(GameStateProvider, networkGame)
{
    NiceMock<MockGameStateProvider> mock;

    ON_CALL(mock, getGameState()).WillByDefault(Return(1));  // GS_LEVEL
    ON_CALL(mock, isPaused()).WillByDefault(Return(false));
    ON_CALL(mock, isNetworkGame()).WillByDefault(Return(true));

    GameComponent component(&mock);

    EXPECT_TRUE(component.isNetworkPlay());
}

// Demonstrate that strict mocks catch unexpected calls
TEST(GameStateProvider, strictMockCatchesUnexpected)
{
    ::testing::StrictMock<MockGameStateProvider> mock;

    // Only set expectations for what we use
    EXPECT_CALL(mock, getGameState()).WillOnce(Return(1));  // GS_LEVEL = 1
    EXPECT_CALL(mock, isPaused()).WillOnce(Return(false));

    GameComponent component(&mock);

    // This should pass - we set expectations for these
    EXPECT_TRUE(component.isPlayable());

    // pollEvent was never expected - strict mock would fail if called
    // (but we don't call it here)
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
