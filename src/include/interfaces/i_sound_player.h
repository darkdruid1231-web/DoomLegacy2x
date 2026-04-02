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
/// \brief Music player interface for dependency injection in tests.
///
/// This interface abstracts music playback to enable mock-based unit testing.
/// Note: ISoundSystem (i_sound.h) handles sound effects; this interface
/// handles music playback.
///
/// Usage in production:
///   ISoundPlayer* player = GetGlobalSoundPlayer(); // returns adapter
///
/// Usage in tests:
///   MockSoundPlayer mock;
///   MusicController ctrl(&mock);  // injects mock

#ifndef INCLUDED_INTERFACES_I_SOUND_PLAYER_H
#define INCLUDED_INTERFACES_I_SOUND_PLAYER_H

class ISoundPlayer {
public:
    virtual ~ISoundPlayer() = default;

    /// Start playing music from a lump name.
    /// \param lumpname Music lump name (e.g., "d_running")
    /// \param loop true to loop the music
    /// \return true on success
    virtual bool startMusic(const char* lumpname, bool loop) = 0;

    /// Stop the currently playing music.
    virtual void stopMusic() = 0;

    /// Pause music playback.
    virtual void pauseMusic() = 0;

    /// Resume paused music.
    virtual void resumeMusic() = 0;

    /// Get the current music lump name.
    /// \return Current music lump name, or nullptr if none
    virtual const char* getCurrentMusic() const = 0;

    /// Set music volume.
    /// \param volume Volume level 0-31
    virtual void setMusicVolume(int volume) = 0;

    /// Update CD audio (if applicable).
    virtual void updateCD() = 0;

    /// Start music by enumeration.
    /// \param musenum Music enum value (mus_* enum)
    /// \param loop true to loop
    /// \return true on success
    virtual bool startMusicByEnum(int musenum, bool loop) = 0;
};

#endif // INCLUDED_INTERFACES_I_SOUND_PLAYER_H
