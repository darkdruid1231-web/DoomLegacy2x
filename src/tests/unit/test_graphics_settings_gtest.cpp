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
/// \brief Google Test tests for GraphicsSettingsComponent using MockCVarProvider.

#include "interfaces/i_cvar.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// Local mock matching MockCVarProvider interface
class MockCVarProviderLocal : public ICVarProvider {
public:
    MOCK_METHOD(int, getInt, (const char* name), (const override));
    MOCK_METHOD(const char*, getString, (const char* name), (const override));
    MOCK_METHOD(bool, setInt, (const char* name, int value), (override));
    MOCK_METHOD(bool, exists, (const char* name), (const override));
};

// Graphics settings component that reads cvar state
class GraphicsSettingsComponent {
public:
    explicit GraphicsSettingsComponent(ICVarProvider* cvar) : cvar_(cvar) {}

    int getRenderMode() const {
        if (!cvar_->exists("vid_rendermode"))
            return 0;
        return cvar_->getInt("vid_rendermode");
    }

    bool isOpenGLEnabled() const {
        return getRenderMode() >= 1;  // 0=SW, 1+=OGL modes
    }

    int getGLColorDepth() const {
        if (!cvar_->exists("gl_colordepth"))
            return 16;
        return cvar_->getInt("gl_colordepth");
    }

    bool isTextureFilterEnabled() const {
        if (!cvar_->exists("gl_texture_filter"))
            return true;  // default on
        return cvar_->getInt("gl_texture_filter") != 0;
    }

    bool setRenderMode(int mode) {
        if (!cvar_->exists("vid_rendermode"))
            return false;
        return cvar_->setInt("vid_rendermode", mode);
    }

private:
    ICVarProvider* cvar_;
};

// Tests

TEST(GraphicsSettings, defaultsToSoftwareRenderer)
{
    NiceMock<MockCVarProviderLocal> mock;

    ON_CALL(mock, exists("vid_rendermode")).WillByDefault(Return(false));

    GraphicsSettingsComponent settings(&mock);
    EXPECT_EQ(settings.getRenderMode(), 0);
    EXPECT_FALSE(settings.isOpenGLEnabled());
}

TEST(GraphicsSettings, detectsOpenGLMode)
{
    NiceMock<MockCVarProviderLocal> mock;

    ON_CALL(mock, exists("vid_rendermode")).WillByDefault(Return(true));
    ON_CALL(mock, getInt("vid_rendermode")).WillByDefault(Return(1));

    GraphicsSettingsComponent settings(&mock);
    EXPECT_TRUE(settings.isOpenGLEnabled());
}

TEST(GraphicsSettings, readsGLColorDepth)
{
    NiceMock<MockCVarProviderLocal> mock;

    ON_CALL(mock, exists("gl_colordepth")).WillByDefault(Return(true));
    ON_CALL(mock, getInt("gl_colordepth")).WillByDefault(Return(32));

    GraphicsSettingsComponent settings(&mock);
    EXPECT_EQ(settings.getGLColorDepth(), 32);
}

TEST(GraphicsSettings, textureFilterDefaultsOn)
{
    NiceMock<MockCVarProviderLocal> mock;

    ON_CALL(mock, exists("gl_texture_filter")).WillByDefault(Return(false));

    GraphicsSettingsComponent settings(&mock);
    EXPECT_TRUE(settings.isTextureFilterEnabled());
}

TEST(GraphicsSettings, textureFilterCanBeDisabled)
{
    NiceMock<MockCVarProviderLocal> mock;

    ON_CALL(mock, exists("gl_texture_filter")).WillByDefault(Return(true));
    ON_CALL(mock, getInt("gl_texture_filter")).WillByDefault(Return(0));

    GraphicsSettingsComponent settings(&mock);
    EXPECT_FALSE(settings.isTextureFilterEnabled());
}

TEST(GraphicsSettings, canSetRenderMode)
{
    NiceMock<MockCVarProviderLocal> mock;

    ON_CALL(mock, exists("vid_rendermode")).WillByDefault(Return(true));
    EXPECT_CALL(mock, setInt("vid_rendermode", 2)).WillOnce(Return(true));

    GraphicsSettingsComponent settings(&mock);
    EXPECT_TRUE(settings.setRenderMode(2));
}

TEST(GraphicsSettings, setRenderModeFailsForUnknown)
{
    NiceMock<MockCVarProviderLocal> mock;

    ON_CALL(mock, exists("vid_rendermode")).WillByDefault(Return(false));

    GraphicsSettingsComponent settings(&mock);
    EXPECT_FALSE(settings.setRenderMode(2));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
