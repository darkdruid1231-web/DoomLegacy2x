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
/// \brief Sound system interface for dependency injection in tests.
///
/// This interface abstracts the SoundSystem functionality to enable mock-based
/// unit testing. Production code uses SoundSystemAdapter which delegates
/// to the global SoundSystem instance.
///
/// Usage in production:
///   ISoundSystem* sound = GetGlobalSoundSystem(); // returns adapter
///
/// Usage in tests:
///   MockSoundSystem mock;
///   MonsterAI m(mock);  // injects mock
///
/// To add a new method to this interface:
///   1. Add the pure virtual method to this class
///   2. Implement in SoundSystemAdapter (audio/s_sound.cpp)
///   3. Override in MockSoundSystem (tests/libraries/mock_sound.cpp)
//-----------------------------------------------------------------------------

#ifndef INCLUDED_INTERFACES_I_SOUND_H
#define INCLUDED_INTERFACES_I_SOUND_H

// Forward declarations
class Actor;

class ISoundSystem {
public:
    virtual ~ISoundSystem() = default;

    /// Start a sound effect at an object's position.
    /// \param mobj Object playing the sound (or nullptr for world sound)
    /// \param sfx_id Sound effect ID (sfx_* enum value)
    /// \param vol Volume 0.0-1.0
    /// \return Channel assigned, or -1 if failed
    virtual int startSound(Actor* mobj, int sfx_id, float vol) = 0;

    /// Stop a sound effect on an object.
    /// \param mobj Object whose sound to stop
    virtual void stopSound(Actor* mobj) = 0;

    /// Set listener (player) position for 3D audio spatialization.
    /// \param x,y,z Position in fixed-point units
    virtual void setListenerPosition(float x, float y, float z) = 0;

    /// Check if a sound is currently playing on a channel.
    /// \param channel Channel number
    /// \return true if sound is active
    virtual bool isChannelPlaying(int channel) const = 0;

    /// Set volume for a specific channel.
    /// \param channel Channel number
    /// \param vol Volume 0.0-1.0
    virtual void setChannelVolume(int channel, float vol) = 0;

    /// Get the number of active sound channels.
    virtual int getActiveChannelCount() const = 0;

    /// Precach a sound effect (load into memory).
    /// \param sfx_id Sound effect ID
    virtual void precacheSound(int sfx_id) = 0;
};

#endif // INCLUDED_INTERFACES_I_SOUND_H
