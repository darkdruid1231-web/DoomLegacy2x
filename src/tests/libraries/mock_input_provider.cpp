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
/// \brief Mock IInputProvider for GTest.

#include "interfaces/i_input_provider.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockInputProvider : public IInputProvider {
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
