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
/// \brief Google Test tests for ICVarProvider using MockCVarProvider.
///
/// This demonstrates the full DI pattern:
///   SettingsComponent (uses ICVarProvider) -> MockCVarProvider (test)

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

// Mock implementation in the test file (for simplicity)
class MockCVarProvider : public ICVarProvider {
public:
    MOCK_METHOD(int, getInt, (const char* name), (const override));
    MOCK_METHOD(const char*, getString, (const char* name), (const override));
    MOCK_METHOD(bool, setInt, (const char* name, int value), (override));
    MOCK_METHOD(bool, exists, (const char* name), (const override));
};

//============================================================================
// Example component that depends on ICVarProvider
//============================================================================

class SettingsComponent {
public:
    explicit SettingsComponent(ICVarProvider* cvar) : cvar_(cvar) {}

    /// Check if a setting is enabled
    bool isEnabled(const char* setting) const {
        return cvar_->getInt(setting) != 0;
    }

    /// Get a volume level (0-100)
    int getVolume() const {
        return cvar_->getInt("soundvolume");
    }

    /// Get the server name
    const char* getServerName() const {
        return cvar_->getString("servername");
    }

    /// Set the master server mode
    bool setMasterServerMode(int mode) {
        return cvar_->setInt("masterservermode", mode);
    }

    /// Check if a cheat is enabled (for testing)
    bool isCheatEnabled(const char* cheat) const {
        return cvar_->getInt(cheat) != 0;
    }

private:
    ICVarProvider* cvar_;
};

//============================================================================
// Tests
//============================================================================

TEST(CVarProvider, soundVolumeSetting)
{
    NiceMock<MockCVarProvider> mock;

    ON_CALL(mock, getInt("soundvolume")).WillByDefault(Return(15));

    SettingsComponent settings(&mock);

    EXPECT_EQ(settings.getVolume(), 15);
}

TEST(CVarProvider, serverName)
{
    NiceMock<MockCVarProvider> mock;

    ON_CALL(mock, getString("servername")).WillByDefault(Return("My Server"));

    SettingsComponent settings(&mock);

    EXPECT_STREQ(settings.getServerName(), "My Server");
}

TEST(CVarProvider, enableSetting)
{
    NiceMock<MockCVarProvider> mock;

    ON_CALL(mock, getInt("cl_advanced")).WillByDefault(Return(1));

    SettingsComponent settings(&mock);

    EXPECT_TRUE(settings.isEnabled("cl_advanced"));
}

TEST(CVarProvider, disabledSetting)
{
    NiceMock<MockCVarProvider> mock;

    ON_CALL(mock, getInt("cl_advanced")).WillByDefault(Return(0));

    SettingsComponent settings(&mock);

    EXPECT_FALSE(settings.isEnabled("cl_advanced"));
}

TEST(CVarProvider, setMasterServerMode)
{
    NiceMock<MockCVarProvider> mock;

    EXPECT_CALL(mock, setInt("masterservermode", 2)).WillOnce(Return(true));

    SettingsComponent settings(&mock);

    EXPECT_TRUE(settings.setMasterServerMode(2));
}

TEST(CVarProvider, cheatEnabled)
{
    NiceMock<MockCVarProvider> mock;

    ON_CALL(mock, getInt("god")).WillByDefault(Return(1));

    SettingsComponent settings(&mock);

    EXPECT_TRUE(settings.isCheatEnabled("god"));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
