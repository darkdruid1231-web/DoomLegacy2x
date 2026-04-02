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
/// \brief Game backend interface for dependency injection in tests.
///
/// This interface abstracts the game loop, state transitions, and server
/// management to enable mock-based unit testing. Production code uses
/// GameBackendAdapter which delegates to the global game state and functions.
///
/// Usage in production:
///   IGameBackend* backend = GetGlobalGameBackend(); // returns adapter
///
/// Usage in tests:
///   MockGameBackend mock;
///   GameRunner runner(&mock);  // injects mock

#ifndef INCLUDED_INTERFACES_I_GAME_BACKEND_H
#define INCLUDED_INTERFACES_I_GAME_BACKEND_H

class IGameBackend {
public:
    virtual ~IGameBackend() = default;

    // --- Game Loop ---

    /// Run the main game loop (D_DoomLoop).
    /// This blocks for the lifetime of the game.
    virtual void runGameLoop() = 0;

    /// Advance the game by elapsed tics.
    /// \param elapsed Number of tics to advance
    virtual void advanceTics(int elapsed) = 0;

    // --- Game State ---

    /// Transition to a new game state.
    /// \param state GS_LEVEL, GS_INTERMISSION, GS_FINALE, etc.
    virtual void setGameState(int state) = 0;

    /// Start the intro sequence.
    virtual void startIntro() = 0;

    /// Advance to the next intro page.
    virtual void advanceIntro() = 0;

    // --- Game Control ---

    /// Pause or unpause the game.
    /// \param on true to pause, false to resume
    virtual void pauseGame(bool on) = 0;

    // --- Rendering ---

    /// Update the display.
    virtual void updateDisplay() = 0;

    /// Draw the current frame.
    virtual void drawFrame() = 0;

    // --- Server ---

    /// Spawn a server with optional mapinfo override.
    /// \param force_mapinfo Use current mapinfo instead of saved
    /// \return true on success
    virtual bool spawnServer(bool force_mapinfo) = 0;

    /// Check if a game is currently in progress.
    virtual bool isPlaying() = 0;

    // --- Exit ---

    /// Quit the game gracefully.
    virtual void quit() = 0;
};

#endif // INCLUDED_INTERFACES_I_GAME_BACKEND_H
