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
/// \brief Game state interface for dependency injection in tests.
///
/// This interface abstracts key game state access to enable mock-based
/// unit testing. Production code uses GameStateProviderAdapter which delegates
/// to the global game state.
///
/// Usage in production:
///   IGameStateProvider* gameState = GetGlobalGameStateProvider();
///
/// Usage in tests:
///   MockGameStateProvider mock;
///   MonsterAI monster(&mock);  // injects mock

#ifndef INCLUDED_INTERFACES_I_GAME_H
#define INCLUDED_INTERFACES_I_GAME_H

// Forward declarations
struct event_t;
struct ticcmd_t;
class PlayerInfo;
class Map;
template<typename T> class vec_t;

// ============================================================================
// Game State Interface
// ============================================================================

/// \brief Game state provider interface.
///
/// Abstracts access to game state (game tic, players, current map) to enable
/// dependency injection and mocking in tests.
class IGameStateProvider {
public:
    virtual ~IGameStateProvider() = default;

    // --- Game State ---

    /// Current game state enum (GS_NULL, GS_LEVEL, GS_INTERMISSION, etc.)
    /// Returns the current gamestate_t value.
    virtual int getGameState() const = 0;

    /// Current game tic counter.
    /// This is the authoritative game time used for logic, not rendering.
    virtual unsigned int getGameTic() const = 0;

    /// Whether the game is currently paused.
    virtual bool isPaused() const = 0;

    /// Whether we are running a network game.
    virtual bool isNetworkGame() const = 0;

    /// Whether this is a multiplayer session (more than 1 player).
    virtual bool isMultiplayer() const = 0;

    // --- Current Map ---

    /// Get the current map instance.
    /// Returns nullptr if no map is currently loaded.
    virtual Map* getCurrentMap() const = 0;

    /// Get the current map's lump number.
    /// Returns -1 if no map is loaded.
    virtual int getCurrentMapLump() const = 0;

    // --- Players ---

    /// Find a player by player number.
    /// Returns nullptr if player not found.
    virtual PlayerInfo* findPlayer(int playernum) const = 0;

    /// Get the number of currently active players.
    virtual int getPlayerCount() const = 0;

    /// Check if a player is alive (in map and health > 0).
    virtual bool isPlayerAlive(const PlayerInfo* p) const = 0;

    /// Get a player's pawn health.
    /// Returns 0 if player has no pawn.
    virtual int getPlayerHealth(const PlayerInfo* p) const = 0;

    /// Get a player's pawn position.
    /// Sets x, y, z to the player's pawn position in fixed-point units.
    /// If player or pawn is null, sets all to 0.
    virtual void getPlayerPosition(const PlayerInfo* p, int& x, int& y, int& z) const = 0;

    /// Get the local player at a given index (0 = first local player).
    /// Used for split-screen and local multiplayer.
    virtual PlayerInfo* getLocalPlayer(int index = 0) const = 0;

    // --- Events ---

    /// Get the next pending event.
    /// Returns true if an event was placed in ev, false if queue was empty.
    virtual bool pollEvent(event_t& ev) = 0;
};

#endif // INCLUDED_INTERFACES_I_GAME_H
