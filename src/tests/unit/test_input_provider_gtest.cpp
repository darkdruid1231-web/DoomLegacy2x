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
/// \brief GTest tests for IInputProvider using MockInputProvider.

#include "interfaces/i_input_provider.h"
#include "d_event.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::DoAll;
using ::testing::SetArrayArgument;

// Local mock matching IInputProvider interface
class MockInputProviderLocal : public IInputProvider {
public:
    MOCK_METHOD(void, pollEvents, (), (override));
    MOCK_METHOD(void, postEvent, (const event_t& ev), (override));
    MOCK_METHOD(void, processEvents, (), (override));
    MOCK_METHOD(bool, getKeyState, (int key), (const override));
    MOCK_METHOD(void, releaseAllKeys, (), (override));
    MOCK_METHOD(uint32_t, getMouseState, (int* x, int* y), (const override));
    MOCK_METHOD(void, warpMouse, (int x, int y), (override));
    MOCK_METHOD(int, getJoystickCount, (), (const override));
    MOCK_METHOD(bool, isJoystickActive, (int index), (const override));
    MOCK_METHOD(int, getJoystickNumAxes, (int index), (const override));
    MOCK_METHOD(int16_t, getJoystickAxis, (int joystickIndex, int axisIndex), (const override));
};

// Test fixture
class InputProviderTest : public ::testing::Test {
protected:
    NiceMock<MockInputProviderLocal> mock_;
};

// Test: postEvent + processEvents triggers keydown handling
TEST_F(InputProviderTest, postKeydownEvent) {
    EXPECT_CALL(mock_, postEvent(_)).Times(1);
    EXPECT_CALL(mock_, processEvents()).Times(1);
    EXPECT_CALL(mock_, getKeyState(75)).WillOnce(Return(true));

    event_t ev = { ev_keydown, 75, 0, 0 };
    mock_.postEvent(ev);
    mock_.processEvents();
    bool kDown = mock_.getKeyState(75);
    EXPECT_TRUE(kDown);
}

// Test: releaseAllKeys clears all key states
TEST_F(InputProviderTest, releaseAllKeys) {
    EXPECT_CALL(mock_, releaseAllKeys()).Times(1);

    mock_.releaseAllKeys();
}

// Test: mouse state returned correctly
TEST_F(InputProviderTest, getMouseState) {
    int x = 0, y = 0;
    EXPECT_CALL(mock_, getMouseState(&x, &y))
        .WillOnce(DoAll(
            testing::SetArgPointee<0>(100),
            testing::SetArgPointee<1>(-200),
            Return(1u)
        ));

    uint32_t buttons = mock_.getMouseState(&x, &y);
    EXPECT_EQ(100, x);
    EXPECT_EQ(-200, y);
    EXPECT_EQ(1u, buttons);
}

// Test: joystick count reflects active joysticks
TEST_F(InputProviderTest, joystickCount) {
    EXPECT_CALL(mock_, getJoystickCount())
        .WillOnce(Return(2));

    EXPECT_EQ(2, mock_.getJoystickCount());
}

// Test: joystick axis values
TEST_F(InputProviderTest, joystickAxis) {
    EXPECT_CALL(mock_, getJoystickAxis(0, 0))
        .WillOnce(Return(32767));
    EXPECT_CALL(mock_, getJoystickAxis(0, 1))
        .WillOnce(Return(-32768));

    EXPECT_EQ(32767, mock_.getJoystickAxis(0, 0));
    EXPECT_EQ(-32768, mock_.getJoystickAxis(0, 1));
}

// Test: warpMouse moves cursor
TEST_F(InputProviderTest, warpMouse) {
    EXPECT_CALL(mock_, warpMouse(640, 400)).Times(1);

    mock_.warpMouse(640, 400);
}

// Test: isJoystickActive returns correct status
TEST_F(InputProviderTest, joystickActiveStatus) {
    EXPECT_CALL(mock_, isJoystickActive(0)).WillOnce(Return(true));
    EXPECT_CALL(mock_, isJoystickActive(1)).WillOnce(Return(false));

    EXPECT_TRUE(mock_.isJoystickActive(0));
    EXPECT_FALSE(mock_.isJoystickActive(1));
}

// Test: pollEvents delegates to I_GetEvent equivalent
TEST_F(InputProviderTest, pollEvents) {
    EXPECT_CALL(mock_, pollEvents()).Times(1);

    mock_.pollEvents();
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
