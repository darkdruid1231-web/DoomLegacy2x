// Emacs style mode select   -*- C++ -*-
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
/// \brief IInputProvider adapter — delegates to SDL/I_GetEvent and game globals.

#include "interfaces/i_input_provider.h"

#include "SDL3/SDL.h"

#include "i_system.h"
#include "d_event.h"
#include "game/g_input.h"

#include <vector>

// External joystick vector from engine/g_input.cpp
extern std::vector<SDL_Joystick*> joysticks;

class InputProviderAdapter : public IInputProvider {
public:
    void pollEvents() override
    {
        I_GetEvent();
    }

    void postEvent(const event_t& ev) override
    {
        D_PostEvent(&ev);
    }

    void processEvents() override
    {
        D_ProcessEvents();
    }

    bool getKeyState(int key) const override
    {
        return G_GetKeyState(key);
    }

    void releaseAllKeys() override
    {
        G_ReleaseKeys();
    }

    uint32_t getMouseState(int* x, int* y) const override
    {
        return SDL_GetMouseState(x, y);
    }

    void warpMouse(int x, int y) override
    {
        SDL_WarpMouseInWindow(NULL, x, y);
    }

    int getJoystickCount() const override
    {
        return static_cast<int>(joysticks.size());
    }

    bool isJoystickActive(int index) const override
    {
        if (index < 0 || index >= static_cast<int>(joysticks.size()))
            return false;
        return SDL_JoystickGetAttached(joysticks[index]) != SDL_FALSE;
    }

    int getJoystickNumAxes(int index) const override
    {
        if (index < 0 || index >= static_cast<int>(joysticks.size()))
            return 0;
        return SDL_JoystickGetNumAxes(joysticks[index]);
    }

    int16_t getJoystickAxis(int joystickIndex, int axisIndex) const override
    {
        if (joystickIndex < 0 || joystickIndex >= static_cast<int>(joysticks.size()))
            return 0;
        SDL_Joystick* joy = joysticks[joystickIndex];
        if (!joy)
            return 0;
        return SDL_JoystickGetAxis(joy, axisIndex);
    }
};

static InputProviderAdapter s_inputProviderAdapter;

IInputProvider* GetGlobalInputProvider()
{
    return &s_inputProviderAdapter;
}
