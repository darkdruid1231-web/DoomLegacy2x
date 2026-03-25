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
/// \brief Mock IRenderer for GTest.

#include "interfaces/i_renderer.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Forward-declared to avoid pulling in g_player.h
class PlayerInfo;

class MockRenderer : public IRenderer {
public:
    MOCK_METHOD(void, beginFrame, (), (override));
    MOCK_METHOD(void, endFrame, (), (override));
    MOCK_METHOD(void, renderPlayerView, (PlayerInfo* player), (override));
    MOCK_METHOD(void, setup2DMode, (), (override));
    MOCK_METHOD(void, setupFullScreen2D, (), (override));
    MOCK_METHOD(void, setPalette, (uint8_t* palette), (override));
    MOCK_METHOD(void, changeResolution, (int width, int height), (override));
    MOCK_METHOD(bool, isReady, (), (const override));
    MOCK_METHOD(int, getRenderMode, (), (const override));
};
