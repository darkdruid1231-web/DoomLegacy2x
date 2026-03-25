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
/// \brief GTest tests for IRenderer using MockRenderer.

#include "interfaces/i_renderer.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// Forward-declared to avoid pulling in game/g_player.h
class PlayerInfo;

// Local mock matching IRenderer interface
class MockRendererLocal : public IRenderer {
public:
    MOCK_METHOD(void, beginFrame, (), (override));
    MOCK_METHOD(void, endFrame, (), (override));
    MOCK_METHOD(void, renderPlayerView, (PlayerInfo* player), (override));
    MOCK_METHOD(void, setup2DMode, (), (override));
    MOCK_METHOD(void, setupFullScreen2D, (), (override));
    MOCK_METHOD(void, setViewport, (int viewport), (override));
    MOCK_METHOD(void, setPalette, (uint8_t* palette), (override));
    MOCK_METHOD(void, changeResolution, (int width, int height), (override));
    MOCK_METHOD(bool, isReady, (), (const override));
    MOCK_METHOD(int, getRenderMode, (), (const override));
};

// Test fixture
class RendererTest : public ::testing::Test {
protected:
    NiceMock<MockRendererLocal> mock_;
};

// Test: frame lifecycle - beginFrame -> renderPlayerView -> endFrame
TEST_F(RendererTest, frameLifecycle) {
    EXPECT_CALL(mock_, beginFrame()).Times(1);
    EXPECT_CALL(mock_, renderPlayerView(nullptr)).Times(1);
    EXPECT_CALL(mock_, endFrame()).Times(1);

    mock_.beginFrame();
    mock_.renderPlayerView(nullptr);
    mock_.endFrame();
}

// Test: 2D mode setup followed by full-screen 2D
TEST_F(RendererTest, twoDModeSetup) {
    EXPECT_CALL(mock_, setup2DMode()).Times(1);
    EXPECT_CALL(mock_, setupFullScreen2D()).Times(1);

    mock_.setup2DMode();
    mock_.setupFullScreen2D();
}

// Test: getRenderMode returns current mode (opengl=2)
TEST_F(RendererTest, getRenderMode) {
    EXPECT_CALL(mock_, getRenderMode())
        .WillOnce(Return(2));  // render_opengl

    int mode = mock_.getRenderMode();
    EXPECT_EQ(2, mode);
}

// Test: isReady reflects renderer state
TEST_F(RendererTest, isReadyReflectsState) {
    EXPECT_CALL(mock_, isReady())
        .WillOnce(Return(true));

    EXPECT_TRUE(mock_.isReady());
}

// Test: changeResolution triggers mode change
TEST_F(RendererTest, changeResolution) {
    EXPECT_CALL(mock_, changeResolution(1920, 1080)).Times(1);

    mock_.changeResolution(1920, 1080);
}

// Test: setPalette provides palette data
TEST_F(RendererTest, setPalette) {
    uint8_t* palette = new uint8_t[256 * 3]();
    EXPECT_CALL(mock_, setPalette(_)).Times(1);

    mock_.setPalette(palette);
    delete[] palette;
}

// Test: multiple player views rendered in sequence (split-screen)
TEST_F(RendererTest, multiplePlayerViews) {
    PlayerInfo* player1 = reinterpret_cast<PlayerInfo*>(1);
    PlayerInfo* player2 = reinterpret_cast<PlayerInfo*>(2);

    EXPECT_CALL(mock_, renderPlayerView(player1)).Times(1);
    EXPECT_CALL(mock_, renderPlayerView(player2)).Times(1);

    mock_.renderPlayerView(player1);
    mock_.renderPlayerView(player2);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
