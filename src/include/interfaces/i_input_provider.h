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
/// \brief IInputProvider interface — abstracts SDL input polling and keyboard/mouse/joystick state.

#ifndef i_input_provider_h
#define i_input_provider_h

#include <cstdint>

struct event_t;  // forward declare

/// \brief Abstracts input subsystem: keyboard, mouse, and joystick polling.
/// \details Follows the INetworkProvider pattern: abstract interface + static adapter
/// accessed via GetGlobalInputProvider().
class IInputProvider {
public:
    virtual ~IInputProvider() = default;

    /// Poll SDL events and push to the internal event queue. Calls D_PostEvent.
    virtual void pollEvents() = 0;

    /// Post a single event directly to the internal queue.
    virtual void postEvent(const event_t& ev) = 0;

    /// Process all pending events through the game event handlers.
    virtual void processEvents() = 0;

    /// Returns true if the given key is currently held down.
    virtual bool getKeyState(int key) const = 0;

    /// Release all currently-held keys.
    virtual void releaseAllKeys() = 0;

    /// Get current mouse state. Returns pressed button mask; x/y filled with position.
    virtual uint32_t getMouseState(int* x, int* y) const = 0;

    /// Warp the mouse cursor to screen position (x, y).
    virtual void warpMouse(int x, int y) = 0;

    /// Number of SDL joysticks currently opened.
    virtual int getJoystickCount() const = 0;

    /// Returns true if the given joystick index is currently attached and active.
    virtual bool isJoystickActive(int index) const = 0;

    /// Returns the number of axes on the given joystick.
    virtual int getJoystickNumAxes(int index) const = 0;

    /// Returns the current value of the given joystick axis (int16_t, -32768 to 32767).
    virtual int16_t getJoystickAxis(int joystickIndex, int axisIndex) const = 0;
};

/// Get the global IInputProvider instance.
IInputProvider* GetGlobalInputProvider();

#endif  // i_input_provider_h
