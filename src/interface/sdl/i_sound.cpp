// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_sound.cpp 514 2007-12-21 16:07:36Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2000-2007 by Doom Legacy team
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// DOOM Source Code License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief OpenAL sound interface for Doom Legacy
///
/// This module provides hardware-accelerated 3D audio using OpenAL.
/// Replaces the SDL_mixer + software mixing approach.
/// Uses SDL_sound for decoding various audio formats.

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// SDL and OpenAL
#include "SDL3/SDL.h"
#ifndef NO_MIXER
#include "SDL3/SDL_mixer.h"
#endif

// FluidSynth for MIDI playback
#ifdef HAVE_FLUIDSYNTH
#include <fluidsynth.h>
#endif

// OpenAL headers (only when available)
#ifdef HAVE_OPENAL
#ifdef _WIN32
#include <AL/al.h>
#include <AL/alc.h>
// ALUT is optional - only include if available
// #include <AL/alut.h>
#elif defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
// #include <OpenAL/alut.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
// #include <AL/alut.h>
#endif
#endif

// SDL_sound for audio decoding
#ifdef HAVE_SDL_SOUND
#include <SDL_sound.h>
#endif

#include "command.h"
#include "cvars.h"
#include "doomdef.h"
#include "doomtype.h"

#include "s_sound.h"  // For soundchannel_t definition

#include "z_zone.h"

#include "i_sound.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fixed.h"
#include "m_misc.h"
#include "m_swap.h"
#include "s_sound.h"
#include "w_wad.h"

#include "s_sound.h"
#include "sounds.h"

#include "qmus2mid.h"

#define MIDBUFFERSIZE 128 * 1024
#define SAMPLERATE 44100 // Hz - OpenAL works best at higher sample rates
#define SAMPLECOUNT 1024 // audio buffer size

// Helper to get float from fixed_t - use .tofloat() if available, otherwise access .val
// Fixed point is 16.16, so divide by 65536
#define FIXED2FLOAT(f) ((float)(f).val / 65536.0f)
#define FIXED2DOUBLE_VAL(v) ((double)(v) / 65536.0)

// Maximum number of OpenAL sound sources (channels)
#define MAX_OPENAL_SOURCES 32
#define MAX_OPENAL_BUFFERS 256

// Pitch to stepping lookup in 16.16 fixed point
static fixed_t steptable[256];

// Volume lookups
static int vol_lookup[128][256];

// MIDI buffer
static byte *mus2mid_buffer;
static int mus2mid_length = 0;

// Flag to use FluidSynth for MIDI playback
static bool use_fluidsynth = false;

// Flags for options
extern bool nosound;
extern bool nomusic;

// Sound provider mode
enum sound_provider_t
{
    SP_SOFTWARE,    // Legacy SDL mixing
    SP_OPENAL       // OpenAL hardware acceleration
};

static sound_provider_t sound_provider = SP_SOFTWARE;
static bool openal_initialized = false;

// OpenAL objects (only when available)
#ifdef HAVE_OPENAL
static ALCdevice *openal_device = NULL;
static ALCcontext *openal_context = NULL;
static ALuint openal_sources[MAX_OPENAL_SOURCES];
static ALuint openal_buffers[MAX_OPENAL_BUFFERS];

// Buffer management
typedef struct al_buffer_s
{
    ALuint buffer;
    ALenum format;
    ALsizei freq;
    ALsizei size;
    bool in_use;
    char lumpname[16];
    const sounditem_t *si_key;  // cache key: which sound item loaded this buffer
} al_buffer_t;

static al_buffer_t al_buffers[MAX_OPENAL_BUFFERS];
static int buffer_count = 0;

// Music streaming
typedef struct music_stream_s
{
    bool playing;
    ALuint source;
    ALuint buffers[2]; // Double buffer for streaming
    int current_buffer;
    bool loop;
} music_stream_t;

static music_stream_t music_stream;
#endif

// FluidSynth MIDI state (when SDL_mixer MIDI fails)
#ifdef HAVE_FLUIDSYNTH
static fluid_synth_t *fluidsynth = nullptr;
static fluid_settings_t *fluidsynth_settings = nullptr;
static SDL_AudioStream *fluid_stream = nullptr;  // SDL3 stream fed by FluidSynth
static fluid_player_t *fluid_player = nullptr;
static bool fluidsynth_ready = false;

// Flag indicating we should use FluidSynth for current music
static bool use_fluid_pcm = false;
#endif

static bool musicStarted = false;
static bool soundStarted = false;

static SDL_AudioSpec audio;

// OpenAL debug output flag - set to true to see detailed OpenAL debug messages
static bool openal_debug = false;

// Console command to toggle OpenAL debug output
static void openaldebug_f()
{
    if (COM.Argc() != 2)
    {
        CONS_Printf("openaldebug 0 - disable OpenAL debug output\n");
        CONS_Printf("openaldebug 1 - enable OpenAL debug output\n");
        return;
    }
    openal_debug = atoi(COM.Argv(1)) != 0;
    CONS_Printf("OpenAL debug output %s\n", openal_debug ? "enabled" : "disabled");
}

// Forward declarations for OpenAL functions
// These are used to call OpenAL functions from non-OpenAL code paths
// When HAVE_OPENAL is defined, these are the actual implementations
// When not defined, these are stubs that do nothing

// music_stream needs to be accessible for OpenAL music control
#ifdef HAVE_OPENAL
extern music_stream_t music_stream;
#else
// Stub structure when OpenAL is not available
struct stub_music_stream_t { bool playing; };
static stub_music_stream_t music_stream = {false};
#endif

// Legacy software mixing support
#ifndef NO_MIXER
// SDL3_mixer structures
static SDL_AudioDeviceID music_device = 0;
static MIX_Mixer *music_mixer = nullptr;
static MIX_Audio *music_audio = nullptr;
static MIX_Track *music_track = nullptr;
#endif

static void I_SetChannels()
{
    int i, j;

    // Pitch table
    for (i = 0; i < 256; i++)
        steptable[i] = float(pow(2.0, ((i - 128) / 64.0)));

    // Volume lookups
    for (i = 0; i < 128; i++)
        for (j = 0; j < 256; j++)
            vol_lookup[i][j] = (i * (j - 128) * 256) / 127;
}

// ====================
// OpenAL Implementation
// ====================
#ifdef HAVE_OPENAL

static bool I_InitOpenAL()
{
    CONS_Printf("OpenAL: Initializing...\n");
    
    // Check for extension support
    const ALCchar* exts = alcGetString(NULL, ALC_EXTENSIONS);
    CONS_Printf("OpenAL: Extensions: %s\n", exts ? exts : "none");
    
    // Open default audio device
    openal_device = alcOpenDevice(NULL);
    if (!openal_device)
    {
        CONS_Printf("OpenAL: Failed to open audio device\n");
        
        // Try to enumerate devices
        const ALCchar* devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
        if (devices && devices[0])
            CONS_Printf("OpenAL: Available devices: %s\n", devices);
        return false;
    }
    
    // Get device info
    const ALCchar* devname = alcGetString(openal_device, ALC_DEVICE_SPECIFIER);
    CONS_Printf("OpenAL: Using device: %s\n", devname ? devname : "default");
    CONS_Printf("OpenAL: Device opened successfully\n");

    // Create context
    const ALint attribs[] =
    {
        ALC_FREQUENCY, SAMPLERATE,
        ALC_REFRESH, 60,
        ALC_SYNC, AL_FALSE,
        0
    };

    openal_context = alcCreateContext(openal_device, attribs);
    if (!openal_context)
    {
        CONS_Printf("OpenAL: Failed to create context\n");
        alcCloseDevice(openal_device);
        openal_device = NULL;
        return false;
    }

    alcMakeContextCurrent(openal_context);

    // Initialize listener at origin, facing -Z (forward)
    ALfloat listener_pos[] = {0.0f, 0.0f, 0.0f};
    ALfloat listener_vel[] = {0.0f, 0.0f, 0.0f};
    ALfloat listener_forward[] = {0.0f, 0.0f, -1.0f};
    ALfloat listener_up[] = {0.0f, 1.0f, 0.0f};

    alListenerfv(AL_POSITION, listener_pos);
    alListenerfv(AL_VELOCITY, listener_vel);
    alListenerfv(AL_ORIENTATION, listener_forward);

    // Generate sound sources
    alGenSources(MAX_OPENAL_SOURCES, openal_sources);
    if (alGetError() != AL_NO_ERROR)
    {
        CONS_Printf("OpenAL: Failed to generate sources\n");
        return false;
    }

    // Initialize all sources as available
    for (int i = 0; i < MAX_OPENAL_SOURCES; i++)
    {
        alSourcei(openal_sources[i], AL_SOURCE_RELATIVE, AL_FALSE);
        alSourcef(openal_sources[i], AL_GAIN, 1.0f);
    }

    // Generate buffers for sound data
    alGenBuffers(MAX_OPENAL_BUFFERS, openal_buffers);
    if (alGetError() != AL_NO_ERROR)
    {
        CONS_Printf("OpenAL: Failed to generate buffers\n");
        return false;
    }

    // Initialize buffer tracking
    memset(al_buffers, 0, sizeof(al_buffers));
    for (int i = 0; i < MAX_OPENAL_BUFFERS; i++)
    {
        al_buffers[i].buffer = openal_buffers[i];
        al_buffers[i].in_use = false;
        al_buffers[i].si_key = NULL;
    }

    // Set distance model for 3D audio
    // Use NONE for now to disable automatic attenuation - we'll handle it manually
    // or use linear for more predictable behavior in Doom's coordinate system
    alDistanceModel(AL_NONE);
    
    // Speed of sound for potential Doppler effect
    alSpeedOfSound(343.3f);

    CONS_Printf("OpenAL: Initialized successfully\n");
    return true;
}

static void I_ShutdownOpenAL()
{
    if (!openal_initialized)
        return;

    // Stop all sources
    alSourceStopv(MAX_OPENAL_SOURCES, openal_sources);

    // Delete sources and buffers
    alDeleteSources(MAX_OPENAL_SOURCES, openal_sources);
    alDeleteBuffers(MAX_OPENAL_BUFFERS, openal_buffers);

    // Cleanup context and device
    if (openal_context)
    {
        alcDestroyContext(openal_context);
        openal_context = NULL;
    }

    if (openal_device)
    {
        alcCloseDevice(openal_device);
        openal_device = NULL;
    }

    openal_initialized = false;
    CONS_Printf("OpenAL: Shutdown complete\n");
}

// Load sound into OpenAL buffer
static int I_LoadSoundToOpenAL(const char *lumpname, const Uint8 *data, size_t size)
{
    // Find free buffer
    int free_idx = -1;
    for (int i = 0; i < MAX_OPENAL_BUFFERS; i++)
    {
        if (!al_buffers[i].in_use)
        {
            free_idx = i;
            break;
        }
    }

    if (free_idx == -1)
    {
        CONS_Printf("OpenAL: No free buffers\n");
        return -1;
    }

    // Try to decode with SDL_sound if available
#ifdef HAVE_SDL_SOUND
    Sound_AudioInfo info;
    info.channels = 1;
    info.format = AUDIO_S16SYS;
    info.rate = SAMPLERATE;

    // This is simplified - actual implementation would need more work
    // to properly decode from lump data
#endif

    // Parse WAV format (simplified)
    const char *riff = (const char *)data;
    if (size >= 12 && strncmp(riff, "RIFF", 4) == 0 && strncmp(riff + 8, "WAVE", 4) == 0)
    {
        // Find format chunk
        size_t offset = 12;
        int16_t channels = 1;
        int16_t bits = 8;
        int32_t rate = 22050;
        const uint8_t *audio_data = NULL;
        size_t audio_size = 0;

        while (offset + 8 < size)
        {
            uint32_t chunk_size = *(uint32_t *)(riff + offset + 4);
            chunk_size = LONG(chunk_size);

            if (strncmp(riff + offset, "fmt ", 4) == 0 && chunk_size >= 16)
            {
                channels = *(int16_t *)(riff + offset + 10);
                bits = *(int16_t *)(riff + offset + 14);
                rate = *(int32_t *)(riff + offset + 12);
                rate = LONG(rate);
            }
            else if (strncmp(riff + offset, "data", 4) == 0)
            {
                audio_data = (const uint8_t *)(riff + offset + 8);
                audio_size = chunk_size;
            }

            offset += 8 + chunk_size;
            if (chunk_size % 2 != 0)
                offset++; // Pad byte
        }

        if (!audio_data || audio_size == 0)
            return -1;

        // Determine format
        ALenum format;
        if (bits == 8)
            format = (channels == 1) ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
        else // 16-bit
            format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

        // Upload to OpenAL buffer
        alBufferData(al_buffers[free_idx].buffer, format, audio_data, audio_size, rate);

        if (alGetError() != AL_NO_ERROR)
        {
            CONS_Printf("OpenAL: Failed to upload buffer data\n");
            return -1;
        }

        al_buffers[free_idx].format = format;
        al_buffers[free_idx].freq = rate;
        al_buffers[free_idx].size = audio_size;
        al_buffers[free_idx].in_use = true;
        strncpy(al_buffers[free_idx].lumpname, lumpname, 15);

        return free_idx;
    }

    // Fallback: Try Doom SFX format
    // Doom SFX: 8-bit unsigned, 22050 Hz, mono
    if (size >= 8)
    {
        // Allocate temporary buffer for conversion
        size_t data_size = size - 8;
        Uint8* converted_data = new Uint8[data_size];
        
        // Convert unsigned to signed correctly:
        // 0-127 maps to 0-127
        // 128-255 maps to -128 to -1
        for (size_t i = 0; i < data_size; i++)
        {
            int unsigned_val = data[8 + i];
            int signed_val = unsigned_val;
            if (signed_val >= 128)
                signed_val -= 256;
            // Now cast back to unsigned for storage (bit pattern is what matters)
            converted_data[i] = (Uint8)(signed_val & 0xFF);
        }
        
        alBufferData(al_buffers[free_idx].buffer, AL_FORMAT_MONO8,
                     converted_data, data_size, 22050);
        
        delete[] converted_data;

        if (alGetError() != AL_NO_ERROR)
            return -1;

        al_buffers[free_idx].format = AL_FORMAT_MONO8;
        al_buffers[free_idx].freq = 22050;
        al_buffers[free_idx].size = data_size;
        al_buffers[free_idx].in_use = true;
        strncpy(al_buffers[free_idx].lumpname, lumpname, 15);

        return free_idx;
    }

    return -1;
}

// Find a free OpenAL source - just find one that's stopped
static int I_GetFreeSource()
{
    for (int i = 0; i < MAX_OPENAL_SOURCES; i++)
    {
        ALint state;
        alGetSourcei(openal_sources[i], AL_SOURCE_STATE, &state);
        if (state == AL_STOPPED || state == AL_INITIAL)
            return i;
    }
    return -1;
}

// OpenAL sound playing
static int I_OpenALStartSound(soundchannel_t *c)
{
    if (!openal_initialized)
        return -1;

    // Ensure our context is current
    alcMakeContextCurrent(openal_context);
    
    // Use the already-converted sound data from the sounditem
    // This is the same data the software mixer uses
    const Uint8* sound_data = (const Uint8*)c->si->sdata;
    size_t data_size = c->si->length;
    
    // Use the original sample rate from the sound item
    // This should be 22050 for most Doom sounds
    unsigned sample_rate = c->si->rate;
    // Fallback to 22050 if rate is 0 or invalid
    if (sample_rate == 0)
        sample_rate = 22050;
    unsigned depth = c->si->depth;
    
    if (!sound_data || data_size == 0)
    {
        CONS_Printf("OpenAL: No sound data\n");
        return -1;
    }

    if (openal_debug)
        CONS_Printf("OpenAL: Sound: rate=%u, depth=%u, size=%zu\n", sample_rate, depth, data_size);

    // Determine OpenAL format based on depth and channels
    // The sdata is already converted, typically to mono
    ALenum format;
    if (depth == 1)
        format = AL_FORMAT_MONO8;
    else if (depth == 2)
        format = AL_FORMAT_MONO16;
    else
    {
        CONS_Printf("OpenAL: Unsupported depth: %u\n", depth);
        return -1;
    }

    // Pass 1: check if this sound is already cached in a buffer
    int buffer_idx = -1;
    for (int i = 0; i < MAX_OPENAL_BUFFERS; i++)
    {
        if (al_buffers[i].in_use && al_buffers[i].si_key == c->si)
        {
            buffer_idx = i;
            break;
        }
    }

    bool need_upload = (buffer_idx == -1);
    if (need_upload)
    {
        // Pass 2a: find a buffer not marked in_use
        for (int i = 0; i < MAX_OPENAL_BUFFERS; i++)
        {
            if (!al_buffers[i].in_use)
            {
                buffer_idx = i;
                break;
            }
        }

        // Pass 2b: recycle a buffer whose source has stopped playing
        if (buffer_idx == -1)
        {
            for (int i = 0; i < MAX_OPENAL_BUFFERS; i++)
            {
                bool still_playing = false;
                for (int j = 0; j < MAX_OPENAL_SOURCES; j++)
                {
                    ALint buf;
                    alGetSourcei(openal_sources[j], AL_BUFFER, &buf);
                    if (buf == (ALint)al_buffers[i].buffer)
                    {
                        ALint state;
                        alGetSourcei(openal_sources[j], AL_SOURCE_STATE, &state);
                        if (state == AL_PLAYING)
                        {
                            still_playing = true;
                            break;
                        }
                    }
                }
                if (!still_playing)
                {
                    // Detach from any stopped source before reuse
                    for (int j = 0; j < MAX_OPENAL_SOURCES; j++)
                    {
                        ALint buf;
                        alGetSourcei(openal_sources[j], AL_BUFFER, &buf);
                        if (buf == (ALint)al_buffers[i].buffer)
                            alSourcei(openal_sources[j], AL_BUFFER, 0);
                    }
                    al_buffers[i].in_use = false;
                    al_buffers[i].si_key = NULL;
                    buffer_idx = i;
                    break;
                }
            }
        }
    }

    if (buffer_idx == -1)
    {
        CONS_Printf("OpenAL: Failed to upload buffer\n");
        return -1;
    }

    if (need_upload)
    {
        alBufferData(al_buffers[buffer_idx].buffer, format, sound_data, data_size, sample_rate);

        if (alGetError() != AL_NO_ERROR)
        {
            CONS_Printf("OpenAL: Failed to upload buffer\n");
            return -1;
        }

        al_buffers[buffer_idx].format = format;
        al_buffers[buffer_idx].freq = sample_rate;
        al_buffers[buffer_idx].size = data_size;
        al_buffers[buffer_idx].in_use = true;
        al_buffers[buffer_idx].si_key = c->si;
    }
    
    if (openal_debug)
        CONS_Printf("OpenAL: Buffer loaded\n");

    // Get source
    int source_idx = I_GetFreeSource();
    if (source_idx == -1)
        return -1;

    ALuint source = openal_sources[source_idx];

    // Stop source if playing
    alSourceStop(source);

    // Bind buffer
    alSourcei(source, AL_BUFFER, al_buffers[buffer_idx].buffer);

    // Set 3D position if this is a positional sound (monsters, items, etc)
    if (c->source.isactor || c->source.mpt)
    {
        // Convert fixed_t coordinates to float using .Float() method (16.16 fixed point)
        float x = c->source.pos.x.Float();
        float y = c->source.pos.y.Float();
        float z = c->source.pos.z.Float();

        alSource3f(source, AL_POSITION, x, y, z);

        // Set velocity for Doppler effect
        float vx = c->source.vel.x.Float();
        float vy = c->source.vel.y.Float();
        float vz = c->source.vel.z.Float();
        alSource3f(source, AL_VELOCITY, vx, vy, vz);

        alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
    }
    else
    {
        // Non-positional (ambient) sound - use stereo panning instead of 3D
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    }

    // Set volume
    float volume = (c->volume * cv_soundvolume.value) / 64.0f;
    alSourcef(source, AL_GAIN, volume);

    // Set pitch - convert from Doom pitch (128=normal) to OpenAL pitch
    // Doom pitch: 64 = one octave down, 128 = normal, 192 = one octave up
    float pitch_value = 1.0f;
    if (c->pitch > 0 && c->pitch < 256)
    {
        pitch_value = (float)pow(2.0, ((c->pitch - 128) / 64.0));
    }
    alSourcef(source, AL_PITCH, pitch_value);
    
    if (openal_debug)
        CONS_Printf("OpenAL: volume=%.2f, pitch=%.2f\n", volume, pitch_value);

    // Play
    ALenum err = alGetError();
    alSourcePlay(source);
    err = alGetError();
    if (err != AL_NO_ERROR)
    {
        CONS_Printf("OpenAL: alSourcePlay failed: %d\n", err);
        return -1;
    }
    
    // Verify it's playing
    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (openal_debug)
        CONS_Printf("OpenAL: Source state: %d\n", state);

    c->playing = true;

    return source_idx;
}

static void I_OpenALStopSound(soundchannel_t *c)
{
    // This is simplified - we'd need to track which source is used by which channel
    // For now, we rely on OpenAL to handle stopped sounds
}

static void I_OpenALUpdateSounds()
{
    if (!openal_initialized)
        return;

    // Update listener position (called from game loop)
    // This should be called with player position
}

void I_SetListenerPosition(float x, float y, float z,
                           float vx, float vy, float vz,
                           float forward_x, float forward_y, float forward_z,
                           float up_x, float up_y, float up_z)
{
    if (!openal_initialized)
        return;

    ALfloat position[] = {x, y, z};
    ALfloat velocity[] = {vx, vy, vz};
    ALfloat orientation[] = {forward_x, forward_y, forward_z, up_x, up_y, up_z};

    alListenerfv(AL_POSITION, position);
    alListenerfv(AL_VELOCITY, velocity);
    alListenerfv(AL_ORIENTATION, orientation);
}

// OpenAL music streaming (simplified)
static bool I_OpenALStartMusic(const Uint8 *data, size_t size, bool loop)
{
    if (!openal_initialized)
        return false;

    // Generate source and buffers for streaming
    alGenSources(1, &music_stream.source);
    alGenBuffers(2, music_stream.buffers);

    music_stream.playing = true;
    music_stream.loop = loop;
    music_stream.current_buffer = 0;

    // This is a placeholder - full implementation would stream decoded audio
    // For now, just load the entire music into a buffer
    // A full implementation would use SDL_sound to decode and stream

    // Generate a simple tone or load actual music data
    // (Implementation depends on music format)

    alSourcei(music_stream.source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    alSourcePlay(music_stream.source);

    return true;
}

static void I_OpenALStopMusic()
{
    if (!openal_initialized || !music_stream.playing)
        return;

    alSourceStop(music_stream.source);
    alDeleteBuffers(2, music_stream.buffers);
    alDeleteSources(1, &music_stream.source);

    music_stream.playing = false;
}

static void I_OpenALSetMusicVolume(int volume)
{
    if (!openal_initialized || !music_stream.playing)
        return;

    // volume is 0-127, convert to 0.0-1.0
    float gain = (volume * 2) / 127.0f;
    alSourcef(music_stream.source, AL_GAIN, gain);
}

#else // HAVE_OPENAL not defined - stub functions

// Stub implementations when OpenAL is not available

static bool I_InitOpenAL()
{
    return false; // OpenAL not available
}

static void I_ShutdownOpenAL()
{
    // Nothing to shutdown
}

static int I_OpenALStartSound(soundchannel_t *c)
{
    (void)c;
    return -1;
}

static void I_OpenALStopSound(soundchannel_t *c)
{
    (void)c;
}

static bool I_OpenALStartMusic(const Uint8 *data, size_t size, bool loop)
{
    (void)data;
    (void)size;
    (void)loop;
    return false;
}

static void I_OpenALStopMusic()
{
    // Nothing to stop
}

static void I_OpenALSetMusicVolume(int volume)
{
    (void)volume;
}

void I_SetListenerPosition(float x, float y, float z,
                           float vx, float vy, float vz,
                           float forward_x, float forward_y, float forward_z,
                           float up_x, float up_y, float up_z)
{
    // No-op when OpenAL is not available
    (void)x; (void)y; (void)z;
    (void)vx; (void)vy; (void)vz;
    (void)forward_x; (void)forward_y; (void)forward_z;
    (void)up_x; (void)up_y; (void)up_z;
}

#endif // HAVE_OPENAL

// ====================
// Legacy Software Implementation
// ====================

void I_SetSfxVolume(int volume)
{
    // Propagate to state variable
}

void soundchannel_t::CalculateParams()
{
    if (sound_provider != SP_SOFTWARE)
        return;

    step = (double(si->rate) / audio.freq) * steptable[pitch];

    float vol = (volume * cv_soundvolume.value) / 64.0f;
    float sep = separation;

    leftvol = vol * (1 - sep * sep);
    sep = 1 - sep;
    rightvol = vol * (1 - sep * sep);

    if (rightvol < 0 || rightvol >= 0.5f)
        I_Error("rightvol out of bounds");
    if (leftvol < 0 || leftvol >= 0.5f)
        I_Error("leftvol out of bounds");

    leftvol_lookup = vol_lookup[int(128 * leftvol)];
    rightvol_lookup = vol_lookup[int(128 * rightvol)];
}

int I_StartSound(soundchannel_t *c)
{
    if (nosound)
        return 0;

    // Use OpenAL if available
    if (sound_provider == SP_OPENAL && openal_initialized)
    {
        return I_OpenALStartSound(c);
    }

    c->samplesize = c->si->depth;
    c->data = (Uint8 *)c->si->sdata;
    c->end = c->data + c->si->length;
    c->stepremainder = 0;
    c->playing = true;

    return 1;
}

void I_StopSound(soundchannel_t *c)
{
    if (sound_provider == SP_OPENAL && openal_initialized)
    {
        I_OpenALStopSound(c);
        return;
    }

    c->playing = false;
}

static void I_UpdateSound_sdl(void *unused, Uint8 *stream, int len)
{
    if (nosound || sound_provider != SP_SOFTWARE)
        return;

    register int dl, dr;
    Sint16 *leftout = (Sint16 *)stream;
    Sint16 *rightout = ((Sint16 *)stream) + 1;
    int step = 2;
    Sint16 *leftend = leftout + len / step;

    int n = S.channels.size();
    for (int i = 0; i < n; i++)
    {
        soundchannel_t *c = &S.channels[i];
        if (c->playing)
        {
            c->CalculateParams();
        }
    }

    while (leftout != leftend)
    {
        dl = *leftout;
        dr = *rightout;

        for (int i = 0; i < n; i++)
        {
            soundchannel_t *c = &S.channels[i];
            if (c->playing)
            {
                c->stepremainder += c->step;

                register unsigned int sample;
                if (c->samplesize == 2)
                {
                    sample = *reinterpret_cast<Sint16 *>(c->data);
                    dl += int(c->leftvol * sample);
                    dr += int(c->rightvol * sample);
                    c->data += 2 * c->stepremainder.floor();
                }
                else
                {
                    sample = *(c->data);
                    dl += c->leftvol_lookup[sample];
                    dr += c->rightvol_lookup[sample];
                    c->data += c->stepremainder.floor();
                }

                c->stepremainder = c->stepremainder.frac();

                if (c->data >= c->end)
                {
                    c->data = NULL;
                    c->playing = false;
                }
            }
        }

        if (dl > 0x7fff)
            *leftout = 0x7fff;
        else if (dl < -0x8000)
            *leftout = -0x8000;
        else
            *leftout = (Sint16)dl;

        if (dr > 0x7fff)
            *rightout = 0x7fff;
        else if (dr < -0x8000)
            *rightout = -0x8000;
        else
            *rightout = (Sint16)dr;

        leftout += step;
        rightout += step;
    }
}

void I_StartupSound()
{
    if (nosound)
        return;

    // Initialize SDL audio first (needed for both OpenAL and SDL_mixer)
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        CONS_Printf("Couldn't initialize SDL Audio: %s\n", SDL_GetError());
        nosound = true;
        return;
    }
    
    audio.freq = SAMPLERATE;
#ifndef SDL3
    audio.format = AUDIO_S16SYS;
    audio.channels = 2;
    audio.samples = SAMPLECOUNT;
#else
    audio.format = SDL_AUDIO_S16;
    audio.channels = 2;
#endif

    // Try to initialize OpenAL first
    if (I_InitOpenAL())
    {
        sound_provider = SP_OPENAL;
        openal_initialized = true;

        // For OpenAL mode, we DON'T open SDL audio for sound effects
        // But we DON'T call SDL_OpenAudio so SDL_mixer can use it for music

        soundStarted = true;
        CONS_Printf("Sound: Using OpenAL audio provider (SDL_mixer for music)\n");

        // Register OpenAL debug command
        COM.AddCommand("openaldebug", openaldebug_f);

        // Still need to initialize music system (SDL_mixer for music + FluidSynth fallback)
        I_InitMusic();

        return;
    }

    CONS_Printf("Sound: OpenAL not available, using software mixing\n");

    // Fallback to SDL audio
    I_SetChannels();

#ifdef NO_MIXER
#ifdef SDL3
    // SDL3 audio uses stream-based API - stub out since OpenAL should be used anyway
    CONS_Printf("SDL3 audio stream API not yet implemented - sound disabled\n");
    nosound = true;
#else
    audio.callback = I_UpdateSound_sdl;
    SDL_OpenAudio(&audio, NULL);
    CONS_Printf("Audio device initialized: %d Hz, %d samples/slice.\n", audio.freq, audio.samples);
    SDL_PauseAudio(0);
    soundStarted = true;
#endif
#else
    sound_provider = SP_SOFTWARE;
#endif
}

// FluidSynth MIDI backend initialization
#ifdef HAVE_FLUIDSYNTH

// SDL3 audio stream get-callback: SDL3_mixer calls this when the music track needs more PCM.
static void SDLCALL FluidSynthStreamCallback(
    void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    if (!fluidsynth || !fluid_player ||
        fluid_player_get_status(fluid_player) != FLUID_PLAYER_PLAYING)
        return;

    int frames = additional_amount / (int)(sizeof(int16_t) * 2);
    int16_t *buf = (int16_t *)SDL_malloc(additional_amount);
    if (!buf) return;

    fluid_synth_write_s16(fluidsynth, frames, buf, 0, 2, buf, 1, 2);
    SDL_PutAudioStreamData(stream, buf, additional_amount);
    SDL_free(buf);
}

static void I_InitFluidSynth()
{
    if (fluidsynth_ready)
        return;

    fluidsynth_settings = new_fluid_settings();
    if (!fluidsynth_settings)
        return;

    fluid_settings_setnum(fluidsynth_settings, "synth.sample-rate", 44100.0);
    fluid_settings_setnum(fluidsynth_settings, "synth.gain", 0.8);
    // No OS audio driver — SDL3 stream handles output
    fluid_settings_setstr(fluidsynth_settings, "audio.driver", "no");

    fluidsynth = new_fluid_synth(fluidsynth_settings);
    if (!fluidsynth)
    {
        delete_fluid_settings(fluidsynth_settings);
        fluidsynth_settings = nullptr;
        return;
    }

    const char *soundfont_paths[] = {
        "gzdoom.sf2",
        "FluidR3_GM.sf2",
        "/usr/share/sounds/sf2/FluidR3_GM.sf2",
        NULL
    };

    bool sf_loaded = false;
    for (int i = 0; soundfont_paths[i]; i++)
    {
        if (fluid_synth_sfload(fluidsynth, soundfont_paths[i], 1) >= 0)
        {
            CONS_Printf("FluidSynth: Loaded SoundFont: %s\n", soundfont_paths[i]);
            sf_loaded = true;
            break;
        }
    }
    if (!sf_loaded)
    {
        CONS_Printf("FluidSynth: No SoundFont found, MIDI disabled\n");
        delete_fluid_synth(fluidsynth);
        fluidsynth = nullptr;
        delete_fluid_settings(fluidsynth_settings);
        fluidsynth_settings = nullptr;
        return;
    }

    // Create SDL3 audio stream — SDL3_mixer reads PCM from this stream via the track
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    spec.freq = 44100;
    fluid_stream = SDL_CreateAudioStream(&spec, &spec);
    if (!fluid_stream)
    {
        CONS_Printf("FluidSynth: Failed to create audio stream, MIDI disabled\n");
        delete_fluid_synth(fluidsynth);
        fluidsynth = nullptr;
        delete_fluid_settings(fluidsynth_settings);
        fluidsynth_settings = nullptr;
        return;
    }
    SDL_SetAudioStreamGetCallback(fluid_stream, FluidSynthStreamCallback, nullptr);

    fluidsynth_ready = true;
    CONS_Printf("FluidSynth: Initialized (SDL3 stream)\n");
}

static void I_ShutdownFluidSynth()
{
    use_fluid_pcm = false;

    if (fluid_stream)
    {
        SDL_DestroyAudioStream(fluid_stream);
        fluid_stream = nullptr;
    }
    if (fluidsynth)
    {
        delete_fluid_synth(fluidsynth);
        fluidsynth = nullptr;
    }
    if (fluidsynth_settings)
    {
        delete_fluid_settings(fluidsynth_settings);
        fluidsynth_settings = nullptr;
    }
    fluidsynth_ready = false;
}

// (I_PreRenderFluidSynthMIDI removed — FluidSynth now streams via SDL_AudioStream)
#if 0
static bool I_PreRenderFluidSynthMIDI(byte *mididata, int midilen)
{
    if (!fluidsynth || !fluidsynth_ready)
        return false;

    if (!mididata || midilen <= 0)
    {
        SDL_Log("I_PreRenderFluidSynthMIDI: ERROR - null or zero-length MIDI data!");
        return false;
    }

    SDL_Log("I_PreRenderFluidSynthMIDI: mididata=%p, midilen=%d", mididata, midilen);

    // Verify synth is working by checking a simple parameter
    double gain = 0.0;
    fluid_settings_getnum(fluidsynth_settings, "synth.gain", &gain);
    SDL_Log("I_PreRenderFluidSynthMIDI: synth.gain = %.2f", gain);
    fluid_synth_set_gain(fluidsynth, 0.8f);
    fluid_settings_getnum(fluidsynth_settings, "synth.gain", &gain);
    SDL_Log("I_PreRenderFluidSynthMIDI: synth.gain after set = %.2f", gain);

    // Check how many SFOUND fonts are loaded
    int sfont_count = fluid_synth_sfcount(fluidsynth);
    SDL_Log("I_PreRenderFluidSynthMIDI: SoundFonts loaded: %d", sfont_count);

    // Debug: print first few bytes of MIDI data
    SDL_Log("I_PreRenderFluidSynthMIDI: First 16 bytes: %02x %02x %02x %02x %02x %02x %02x %02x ...",
            mididata[0], mididata[1], mididata[2], mididata[3],
            mididata[4], mididata[5], mididata[6], mididata[7]);
    // Also print last bytes
    SDL_Log("I_PreRenderFluidSynthMIDI: Last 8 bytes: %02x %02x %02x %02x %02x %02x %02x %02x",
            mididata[midilen-8], mididata[midilen-7], mididata[midilen-6], mididata[midilen-5],
            mididata[midilen-4], mididata[midilen-3], mididata[midilen-2], mididata[midilen-1]);

    // Parse MIDI header info
    // MIDI header structure:
    //   0-3: "MThd"
    //   4-7: header length (big-endian, usually 6)
    //   8-9: format (big-endian)
    //   10-11: num_tracks (big-endian)
    //   12-13: division (big-endian)
    if (midilen >= 14 && memcmp(mididata, "MThd", 4) == 0)
    {
        Uint16 format = (mididata[8] << 8) | mididata[9];
        Uint16 num_tracks = (mididata[10] << 8) | mididata[11];
        Uint16 division = (mididata[12] << 8) | mididata[13];
        SDL_Log("I_PreRenderFluidSynthMIDI: MIDI format=%d, tracks=%d, division=%d",
                format, num_tracks, division);

        // Find ALL MTrk chunks
        size_t off = 14;
        int track_num = 0;
        while (off + 8 < (size_t)midilen && track_num < num_tracks)
        {
            if (memcmp(mididata + off, "MTrk", 4) == 0)
            {
                Uint32 tlen = (mididata[off+4] << 24) | (mididata[off+5] << 16) |
                             (mididata[off+6] << 8) | mididata[off+7];
                SDL_Log("I_PreRenderFluidSynthMIDI: MTrk %d at offset %zu, len=%u",
                        track_num, off, tlen);

                // Dump first 40 bytes of track data (after 8-byte header)
                if (tlen > 0 && off + 8 + 40 <= (size_t)midilen)
                {
                    SDL_Log("  Track %d bytes 0-39: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                            track_num,
                            mididata[off+8], mididata[off+9], mididata[off+10], mididata[off+11],
                            mididata[off+12], mididata[off+13], mididata[off+14], mididata[off+15],
                            mididata[off+16], mididata[off+17], mididata[off+18], mididata[off+19],
                            mididata[off+20], mididata[off+21], mididata[off+22], mididata[off+23],
                            mididata[off+24], mididata[off+25], mididata[off+26], mididata[off+27],
                            mididata[off+28], mididata[off+29], mididata[off+30], mididata[off+31],
                            mididata[off+32], mididata[off+33], mididata[off+34], mididata[off+35],
                            mididata[off+36], mididata[off+37], mididata[off+38], mididata[off+39],
                            mididata[off+40], mididata[off+41], mididata[off+42], mididata[off+43],
                            mididata[off+44], mididata[off+45], mididata[off+46], mididata[off+47]);
                }

                off += 8 + tlen;  // skip to next track
                track_num++;
            }
            else
            {
                off++;
            }
        }
        SDL_Log("I_PreRenderFluidSynthMIDI: Found %d MTrk chunks total", track_num);
    }

    // Free any existing buffers
    if (fluid_wav_buffer)
    {
        SDL_free(fluid_wav_buffer);
        fluid_wav_buffer = nullptr;
    }
    if (fluid_pcm_buffer)
    {
        SDL_free(fluid_pcm_buffer);
        fluid_pcm_buffer = nullptr;
    }

    const int sample_rate = 44100;
    const int channels = 2;  // Stereo

    // Create a player to track timing
    fluid_player_t *player = new_fluid_player(fluidsynth);
    if (!player)
    {
        SDL_Log("I_PreRenderFluidSynthMIDI: Failed to create player");
        return false;
    }

    // Load the MIDI data
    SDL_Log("I_PreRenderFluidSynthMIDI: Adding MIDI data to player...");
    if (fluid_player_add_mem(player, mididata, midilen) != FLUID_OK)
    {
        SDL_Log("I_PreRenderFluidSynthMIDI: Failed to add MIDI data");
        delete_fluid_player(player);
        return false;
    }
    SDL_Log("I_PreRenderFluidSynthMIDI: MIDI data added successfully");

    // Pre-allocate a large buffer (max ~6 minutes at 44.1kHz stereo = ~64MB)
    const size_t max_samples = sample_rate * channels * 60 * 6;
    fluid_pcm_buffer = (byte *)SDL_malloc(max_samples * sizeof(short));
    if (!fluid_pcm_buffer)
    {
        SDL_Log("I_PreRenderFluidSynthMIDI: Failed to allocate PCM buffer");
        delete_fluid_player(player);
        return false;
    }

    short *pcm_out = (short *)fluid_pcm_buffer;
    size_t samples_written = 0;

    // For now, just do a simple test - use FluidSynth's program change to verify the synth works
    SDL_Log("I_PreRenderFluidSynthMIDI: Testing synth with program change...");

    fluid_player_stop(player);
    delete_fluid_player(player);
    player = nullptr;

    // Try playing a note directly via FluidSynth API to verify sound works
    SDL_Log("I_PreRenderFluidSynthMIDI: Playing test note on channel 0...");
    fluid_synth_program_change(fluidsynth, 0, 0);  // Set piano on channel 0
    fluid_synth_noteon(fluidsynth, 0, 60, 127);  // Play C4, velocity 127
    SDL_Delay(500);  // Wait 500ms
    fluid_synth_noteoff(fluidsynth, 0, 60);  // Release
    SDL_Log("I_PreRenderFluidSynthMIDI: Test note played");

    // Now render 2 seconds of silence (since we can't easily render without player timing)
    SDL_Log("I_PreRenderFluidSynthMIDI: Rendering 2 seconds of silence...");
    int test_samples = sample_rate * 2;
    memset(fluid_pcm_buffer, 0, test_samples * channels * sizeof(short));
    samples_written = test_samples * channels;
    SDL_Log("I_PreRenderFluidSynthMIDI: Fallback render complete: %zu samples", samples_written);

    fluid_player_stop(player);
    delete_fluid_player(player);
    player = nullptr;

    fluid_pcm_buffer_size = samples_written * sizeof(short);

    SDL_Log("I_PreRenderFluidSynthMIDI: Rendered %zu samples (%zu bytes, ~%.1f seconds)",
            samples_written, fluid_pcm_buffer_size,
            (double)samples_written / (sample_rate * channels));

    // Create WAV header + data
    // WAV header is 44 bytes
    const size_t wav_header_size = 44;
    fluid_wav_buffer_size = wav_header_size + fluid_pcm_buffer_size;
    fluid_wav_buffer = (byte *)SDL_malloc(fluid_wav_buffer_size);
    if (!fluid_wav_buffer)
    {
        SDL_Log("I_PreRenderFluidSynthMIDI: Failed to allocate WAV buffer");
        SDL_free(fluid_pcm_buffer);
        fluid_pcm_buffer = nullptr;
        return false;
    }

    // Build WAV header
    byte *wav = fluid_wav_buffer;
    size_t offset = 0;

    // RIFF header
    memcpy(wav + offset, "RIFF", 4); offset += 4;
    Uint32 file_size = 36 + fluid_pcm_buffer_size;
    memcpy(wav + offset, &file_size, 4); offset += 4;
    memcpy(wav + offset, "WAVE", 4); offset += 4;

    // fmt chunk
    memcpy(wav + offset, "fmt ", 4); offset += 4;
    Uint32 fmt_size = 16;  // PCM format
    memcpy(wav + offset, &fmt_size, 4); offset += 4;
    Uint16 audio_format = 1;  // PCM
    memcpy(wav + offset, &audio_format, 2); offset += 2;
    Uint16 num_channels = channels;
    memcpy(wav + offset, &num_channels, 2); offset += 2;
    Uint32 sample_rate_val = sample_rate;
    memcpy(wav + offset, &sample_rate_val, 4); offset += 4;
    Uint32 byte_rate = sample_rate * channels * sizeof(short);
    memcpy(wav + offset, &byte_rate, 4); offset += 4;
    Uint16 block_align = channels * sizeof(short);
    memcpy(wav + offset, &block_align, 2); offset += 2;
    Uint16 bits_per_sample = 16;
    memcpy(wav + offset, &bits_per_sample, 2); offset += 2;

    // data chunk
    memcpy(wav + offset, "data", 4); offset += 4;
    Uint32 data_size = fluid_pcm_buffer_size;
    memcpy(wav + offset, &data_size, 4); offset += 4;

    // Copy PCM data after header
    memcpy(wav + offset, fluid_pcm_buffer, fluid_pcm_buffer_size);

    SDL_Log("I_PreRenderFluidSynthMIDI: Created WAV buffer, total size = %zu bytes", fluid_wav_buffer_size);
    return true;
}
#endif // 0
#endif // HAVE_FLUIDSYNTH

void I_InitMusic()
{
    if (nosound)
    {
        nomusic = true;
        return;
    }

    // In OpenAL mode, we can still use SDL_mixer for music
    // by opening its own audio device

    // Software mode - use SDL_mixer for music
#ifdef NO_MIXER
    nomusic = true;
#else
    // Open SDL audio device for music playback
    SDL_AudioSpec spec;
    spec.freq = audio.freq;
    spec.format = audio.format;
    spec.channels = audio.channels;

    // Debug: list available audio playback devices
    int num_devices = 0;
    SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&num_devices);
    CONS_Printf("Available audio playback devices: %d\n", num_devices);
    for (int i = 0; i < num_devices; i++)
    {
        CONS_Printf("  Device %d: %s\n", i, SDL_GetAudioDeviceName(devices[i]));
    }
    if (devices) SDL_free(devices);

    // Open a playback audio device for music with specific format
    SDL_AudioSpec desired = {0};
    desired.freq = 44100;
    desired.format = SDL_AUDIO_S16;
    desired.channels = 2;
    SDL_Log("I_InitMusic: Opening audio device with freq=%d, format=0x%x, channels=%d",
            desired.freq, desired.format, desired.channels);
    music_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired);
    if (music_device == 0)
    {
        SDL_Log("I_InitMusic: FAILED to open audio device: %s", SDL_GetError());
        CONS_Printf("Unable to open audio device for music: %s\n", SDL_GetError());
        nomusic = true;
        return;
    }
    SDL_Log("I_InitMusic: Audio device opened successfully, device ID=%d", music_device);

    // Get the actual device format
    SDL_AudioSpec dev_spec;
    if (!SDL_GetAudioDeviceFormat(music_device, &dev_spec, NULL))
    {
        SDL_Log("I_InitMusic: Failed to get device format: %s", SDL_GetError());
        CONS_Printf("Failed to get audio device format: %s\n", SDL_GetError());
        SDL_CloseAudioDevice(music_device);
        nomusic = true;
        return;
    }
    SDL_Log("I_InitMusic: Device format: freq=%d, format=0x%x, channels=%d",
            dev_spec.freq, dev_spec.format, dev_spec.channels);

    // Initialize SDL3_mixer
    if (!MIX_Init())
    {
        SDL_Log("I_InitMusic: FAILED to initialize SDL3_mixer: %s", SDL_GetError());
        CONS_Printf("Unable to initialize SDL3_mixer: %s\n", SDL_GetError());
        SDL_CloseAudioDevice(music_device);
        nomusic = true;
        return;
    }
    SDL_Log("I_InitMusic: SDL3_mixer initialized successfully");

    // Debug: list available audio decoders
    int num_decoders = MIX_GetNumAudioDecoders();
    SDL_Log("Available audio decoders: %d", num_decoders);
    for (int i = 0; i < num_decoders; i++)
    {
        SDL_Log("  Decoder %d: %s", i, MIX_GetAudioDecoder(i));
    }

    // Create mixer connected to the audio device
    music_mixer = MIX_CreateMixerDevice(music_device, &dev_spec);
    if (!music_mixer)
    {
        SDL_Log("I_InitMusic: FAILED to create mixer: %s", SDL_GetError());
        CONS_Printf("Unable to create SDL3_mixer: %s\n", SDL_GetError());
        SDL_CloseAudioDevice(music_device);
        nomusic = true;
        return;
    }
    SDL_Log("I_InitMusic: Mixer created successfully");

    // Note: SDL3 audio devices start in unpaused state, no need to unpause
    CONS_Printf("Music audio device opened: %d Hz, %d channels.\n", dev_spec.freq, dev_spec.channels);

    mus2mid_buffer = (byte *)Z_Malloc(MIDBUFFERSIZE, PU_MUSIC, NULL);
#endif

    // Initialize FluidSynth as a fallback for MIDI playback
#ifdef HAVE_FLUIDSYNTH
    I_InitFluidSynth();
#endif

    CONS_Printf("Music initialized.\n");
    musicStarted = true;
}

void I_ShutdownSound()
{
    // Always clean up SDL_mixer resources first if they exist
#ifndef NO_MIXER
    if (music_track)
    {
        MIX_StopTrack(music_track, 0);
        MIX_DestroyTrack(music_track);
        music_track = nullptr;
    }
    if (music_audio)
    {
        MIX_DestroyAudio(music_audio);
        music_audio = nullptr;
    }
    if (music_mixer)
    {
        MIX_DestroyMixer(music_mixer);
        music_mixer = nullptr;
    }
    if (music_device != 0)
    {
        SDL_CloseAudioDevice(music_device);
        music_device = 0;
    }
    // Quit SDL_mixer (this destroys all remaining SDL_mixer objects)
    MIX_Quit();
#endif

    // Clean up FluidSynth
#ifdef HAVE_FLUIDSYNTH
    if (fluidsynth_ready)
    {
        I_ShutdownFluidSynth();
    }
#endif

    if (sound_provider == SP_OPENAL && openal_initialized)
    {
        I_ShutdownOpenAL();
        soundStarted = false;
        musicStarted = false;
        return;
    }

    if (!soundStarted)
        return;

    CONS_Printf("I_ShutdownSound: ");

#ifdef NO_MIXER
#ifndef SDL3
    SDL_CloseAudio();
#endif
#else
    // SDL_mixer resources already cleaned up above
#endif

    CONS_Printf("shut down\n");
    soundStarted = false;

    if (!musicStarted)
        return;

    Z_Free(mus2mid_buffer);
    CONS_Printf("I_ShutdownMusic: shut down\n");
    musicStarted = false;
}

void I_PlaySong(int handle, int looping)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    SDL_Log("I_PlaySong called: nomusic=%d, music_mixer=%p, music_audio=%p, looping=%d, use_fluid_pcm=%d",
            nomusic, (void*)music_mixer, (void*)music_audio, looping, use_fluid_pcm);
    if (nomusic)
        return;

    // Handle FluidSynth MIDI — route through music_track for unified volume control
#ifdef HAVE_FLUIDSYNTH
    if (use_fluid_pcm && fluid_player)
    {
        if (!music_mixer || !fluid_stream)
        {
            SDL_Log("I_PlaySong: FluidSynth path missing mixer or stream");
            return;
        }

        // Destroy any previous track
        if (music_track)
        {
            MIX_StopTrack(music_track, 0);
            MIX_DestroyTrack(music_track);
            music_track = nullptr;
        }

        music_track = MIX_CreateTrack(music_mixer);
        if (!music_track)
        {
            SDL_Log("I_PlaySong: Failed to create FluidSynth track: %s", SDL_GetError());
            return;
        }

        MIX_SetTrackAudioStream(music_track, fluid_stream);

        fluid_player_set_loop(fluid_player, looping ? -1 : 0);
        fluid_player_play(fluid_player);

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_FRAMES_NUMBER, 30);
        if (!MIX_PlayTrack(music_track, props))
            SDL_Log("I_PlaySong: MIX_PlayTrack (FluidSynth) failed: %s", SDL_GetError());
        else
            SDL_Log("I_PlaySong: FluidSynth MIDI started via SDL3 mixer track");
        SDL_DestroyProperties(props);
        return;
    }
#endif

    if (!music_mixer || !music_audio)
    {
        SDL_Log("I_PlaySong: returning early - mixer or audio is NULL");
        return;
    }

    // Destroy previous track if exists
    if (music_track)
    {
        MIX_StopTrack(music_track, 0);
        MIX_DestroyTrack(music_track);
    }

    // Create new track
    music_track = MIX_CreateTrack(music_mixer);
    if (!music_track)
    {
        SDL_Log("I_PlaySong: FAILED to create track: %s", SDL_GetError());
        CONS_Printf("Failed to create music track: %s\n", SDL_GetError());
        return;
    }
    SDL_Log("I_PlaySong: Track created successfully");

    // Set the audio on the track
    if (!MIX_SetTrackAudio(music_track, music_audio))
    {
        SDL_Log("I_PlaySong: FAILED to set track audio: %s", SDL_GetError());
        CONS_Printf("Failed to set track audio: %s\n", SDL_GetError());
        MIX_DestroyTrack(music_track);
        music_track = nullptr;
        return;
    }
    SDL_Log("I_PlaySong: Track audio set successfully");

    // Set looping - -1 means infinite, 0 means play once, positive means that many times
    int loops = looping ? -1 : 0;
    MIX_SetTrackLoops(music_track, loops);

    // Set fade-in frames (500ms at typical sample rate ~60fps)
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_FRAMES_NUMBER, 30);

    if (!MIX_PlayTrack(music_track, props))
    {
        SDL_Log("I_PlaySong: FAILED to play track: %s", SDL_GetError());
        CONS_Printf("Failed to play music: %s\n", SDL_GetError());
        MIX_DestroyTrack(music_track);
        music_track = nullptr;
    }
    else
    {
        SDL_Log("I_PlaySong: Music started successfully!");
    }
    SDL_DestroyProperties(props);
#endif
}

void I_PauseSong(int handle)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return;

    // FluidSynth pre-rendered PCM uses SDL_mixer pause
#ifdef HAVE_FLUIDSYNTH
    if (use_fluid_pcm)
    {
        // Fall through to SDL_mixer pause
    }
#endif

    if (music_track)
    {
        MIX_PauseTrack(music_track);
    }
#endif
}

void I_ResumeSong(int handle)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return;

    // FluidSynth pre-rendered PCM uses SDL_mixer resume
#ifdef HAVE_FLUIDSYNTH
    if (use_fluid_pcm)
    {
        // Fall through to SDL_mixer resume
    }
#endif

    if (music_track)
    {
        MIX_ResumeTrack(music_track);
    }
#endif
}

void I_StopSong(int handle)
{
    // Handle FluidSynth player stop
#ifdef HAVE_FLUIDSYNTH
    if (use_fluid_pcm && fluid_player)
    {
        SDL_Log("I_StopSong: Stopping FluidSynth player");
        fluid_player_stop(fluid_player);
        if (music_track)
            MIX_StopTrack(music_track, 30);
        return;
    }
#endif

    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return;

    if (music_track)
    {
        // Fade out over 500ms (~30 frames)
        MIX_StopTrack(music_track, 30);
    }
#endif
}

void I_UnRegisterSong(int handle)
{
    // Handle FluidSynth player cleanup
#ifdef HAVE_FLUIDSYNTH
    if (use_fluid_pcm && fluid_player)
    {
        SDL_Log("I_UnRegisterSong: Cleaning up FluidSynth player");
        fluid_player_stop(fluid_player);
        delete_fluid_player(fluid_player);
        fluid_player = nullptr;
        use_fluid_pcm = false;
        if (music_track)
        {
            MIX_DestroyTrack(music_track);
            music_track = nullptr;
        }
        SDL_Log("I_UnRegisterSong: FluidSynth cleanup complete");
        return;
    }
#endif

    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return;

    if (music_track)
    {
        MIX_StopTrack(music_track, 0);
        MIX_DestroyTrack(music_track);
        music_track = nullptr;
    }
    if (music_audio)
    {
        MIX_DestroyAudio(music_audio);
        music_audio = nullptr;
    }
#endif
}

int I_RegisterSong(void *data, int len)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    SDL_Log("I_RegisterSong called: data=%p, len=%d, nomusic=%d", data, len, nomusic);
    if (nomusic)
        return 0;

    if (music_audio)
    {
        I_Error("Two registered pieces of music simultaneously!\n");
    }

    byte *bdata = static_cast<byte *>(data);
    SDL_IOStream *rwop = nullptr;

    // Check if this is a MUS file that needs conversion
    bool is_mus = (memcmp(data, MUSMAGIC, 4) == 0);
    bool is_midi = (memcmp(data, "MThd", 4) == 0);

    SDL_Log("I_RegisterSong: data[0-3]=%02x%02x%02x%02x, is_mus=%d, is_midi=%d",
            bdata[0], bdata[1], bdata[2], bdata[3], is_mus, is_midi);

    // midlength is hoisted here so the FluidSynth fallback can reuse it
    int err;
    Uint32 midlength = 0;

    if (is_mus)
    {
        // Debug: show MUS header values (little-endian parsing)
        Uint16 score_len = bdata[4] | (bdata[5] << 8);
        Uint16 score_start = bdata[6] | (bdata[7] << 8);
        Uint16 channels = bdata[8] | (bdata[9] << 8);
        // Also try big-endian parsing
        Uint16 score_len_be = (bdata[4] << 8) | bdata[5];
        Uint16 score_start_be = (bdata[6] << 8) | bdata[7];
        SDL_Log("I_RegisterSong: MUS header: len=%d, scoreLen_LE=%d, scoreStart_LE=%d, channels_LE=%d",
                len, score_len, score_start, channels);
        SDL_Log("I_RegisterSong: MUS header (BE): scoreLen_BE=%d, scoreStart_BE=%d",
                score_len_be, score_start_be);

        // Try big-endian parsing if scoreStart seems wrong
        if (score_start > len) {
            SDL_Log("I_RegisterSong: scoreStart > len, trying big-endian MUS");
            // Create a byte-swapped copy
            byte mus_be[16 + 65536]; // header + max MUS size
            memcpy(mus_be, bdata, len);
            // Byte-swap the Uint16 fields after the 4-byte header
            for (int i = 4; i < len && i < 16; i += 2) {
                byte t = mus_be[i];
                mus_be[i] = mus_be[i+1];
                mus_be[i+1] = t;
            }
            err = qmus2mid(mus_be, mus2mid_buffer, 96, 64, 0, len, MIDBUFFERSIZE, &midlength);
            if (err == 0) {
                SDL_Log("I_RegisterSong: Big-endian MUS conversion succeeded, midilen=%d", midlength);
                rwop = SDL_IOFromConstMem(mus2mid_buffer, midlength);
            } else {
                SDL_Log("I_RegisterSong: Big-endian MUS conversion failed: error %d", err);
                nomusic = true;
                return 0;
            }
        } else {
            // Try with current parsing
            if ((err = qmus2mid(bdata, mus2mid_buffer, 96, 64, 0, len, MIDBUFFERSIZE, &midlength)) != 0)
            {
                SDL_Log("I_RegisterSong: MUS to MIDI conversion failed: error %d", err);
                CONS_Printf("Cannot convert MUS to MIDI: error %d.\n", err);
                return 0;
            }
            SDL_Log("I_RegisterSong: MUS converted to MIDI, length=%d", midlength);

        // Debug: show first 70 bytes of MIDI output to see track 0
        SDL_Log("I_RegisterSong: MIDI bytes 0-67:");
        for (int i = 0; i < 68 && i < (int)midlength; i++) {
            SDL_Log("  [%d] = 0x%02x", i, mus2mid_buffer[i]);
        }
            rwop = SDL_IOFromConstMem(mus2mid_buffer, midlength);
        }
    }
    else if (is_midi)
    {
        SDL_Log("I_RegisterSong: Data is already MIDI format");
        rwop = SDL_IOFromConstMem(data, len);
    }
    else
    {
        SDL_Log("I_RegisterSong: Unknown music format, trying as raw MIDI");
        rwop = SDL_IOFromConstMem(data, len);
    }

    if (!rwop)
    {
        SDL_Log("I_RegisterSong: Failed to create SDL_IOStream: %s", SDL_GetError());
        CONS_Printf("Couldn't create SDL_IOStream for music: %s\n", SDL_GetError());
        return 0;
    }

    // Load audio using SDL3_mixer
    SDL_Log("I_RegisterSong: Loading audio with MIX_LoadAudio_IO, music_mixer=%p", (void*)music_mixer);
    music_audio = MIX_LoadAudio_IO(music_mixer, rwop, true, true); // predecode=true, closeio=true
    if (!music_audio)
    {
        // SDL3_mixer doesn't support MIDI format (no MIDI decoder available)
        // MUS files are converted to MIDI but require a SoundFont to play
        // Try FluidSynth as a fallback for MIDI playback
        SDL_Log("I_RegisterSong: SDL3_mixer cannot play MIDI, trying FluidSynth...");

#ifdef HAVE_FLUIDSYNTH
        if (fluidsynth_ready && fluidsynth)
        {
            SDL_Log("I_RegisterSong: FluidSynth is ready, attempting MIDI playback");
            // Determine MIDI data and length
            byte *mididata = nullptr;
            int midilen = 0;

            if (is_mus)
            {
                // MIDI already in mus2mid_buffer from the conversion above
                mididata = mus2mid_buffer;
                midilen = (int)midlength;
                SDL_Log("I_RegisterSong: Reusing MUS->MIDI from mus2mid_buffer, midilen=%d", midilen);
            }
            else if (is_midi)
            {
                SDL_Log("I_RegisterSong: Data is MIDI, passing directly to FluidSynth");
                mididata = bdata;
                midilen = len;
            }
            else
            {
                SDL_Log("I_RegisterSong: Data is unknown format, trying as raw MIDI");
                mididata = bdata;
                midilen = len;
            }

            // Debug: write MIDI data to file for inspection
            {
                static int dbg_count = 0;
                char fname[64];
                snprintf(fname, sizeof(fname), "C:/doomlegacy-windows/debug_midi_%d.bin", dbg_count++);
                FILE *f = fopen(fname, "wb");
                if (f) {
                    fwrite(mididata, 1, midilen, f);
                    fclose(f);
                    SDL_Log("I_RegisterSong: Wrote MIDI debug file: %s (%d bytes)", fname, midilen);
                } else {
                    SDL_Log("I_RegisterSong: Failed to write MIDI debug file: %s", fname);
                }
            }

            // Add MIDI to FluidSynth player - the player uses FluidSynth's audio driver
            // But first, fix track lengths in the MIDI data since qmus2mid may have incorrect lengths
            SDL_Log("I_RegisterSong: Pre-track-fix: mididata=%p, midilen=%d", (void*)mididata, midilen);
            int fixed_len = midilen;
            byte *fixed_mididata = (byte*)SDL_malloc(midilen);
            if (fixed_mididata)
            {
                memcpy(fixed_mididata, mididata, midilen);
                SDL_Log("I_RegisterSong: Allocated fixed_mididata=%p", (void*)fixed_mididata);
                // Find all MTrk chunks and recalculate their lengths
                // MIDI structure: MThd(14) + MTrk chunks...
                size_t off = 14;
                int track_num = 0;
                while (off + 8 < (size_t)midilen)
                {
                    if (memcmp(fixed_mididata + off, "MTrk", 4) == 0)
                    {
                        Uint32 declared_len = (fixed_mididata[off+4] << 24) | (fixed_mididata[off+5] << 16) |
                                             (fixed_mididata[off+6] << 8) | fixed_mididata[off+7];
                        SDL_Log("I_RegisterSong: Track %d at off=%zu, declared_len=%u", track_num, off, declared_len);
                        // Scan for end-of-track meta event (FF 2F 00) within the declared length
                        // Simply scan byte by byte looking for FF 2F 00
                        size_t scan_end = off + 8 + declared_len;
                        size_t actual_len = declared_len;
                        for (size_t scan_off = off + 8; scan_off < scan_end - 2; scan_off++)
                        {
                            if (fixed_mididata[scan_off] == 0xFF && fixed_mididata[scan_off+1] == 0x2F &&
                                fixed_mididata[scan_off+2] == 0x00)
                            {
                                // Found end-of-track: FF 2F 00
                                actual_len = (scan_off - off - 8) + 3; // +3 for the FF 2F 00 itself
                                SDL_Log("I_RegisterSong:   Found FF2F00 at scan_off=%zu, actual_len=%zu", scan_off, actual_len);
                                break;
                            }
                        }
                        if (actual_len != declared_len)
                        {
                            SDL_Log("I_RegisterSong: Fixing track %d length: declared=%u, actual=%zu",
                                    track_num, declared_len, actual_len);
                            fixed_mididata[off+4] = (actual_len >> 24) & 0xFF;
                            fixed_mididata[off+5] = (actual_len >> 16) & 0xFF;
                            fixed_mididata[off+6] = (actual_len >> 8) & 0xFF;
                            fixed_mididata[off+7] = actual_len & 0xFF;
                        }
                        else
                        {
                            SDL_Log("I_RegisterSong: Track %d length OK (no fix needed)", track_num);
                        }
                        off += 8 + declared_len;
                        track_num++;
                    }
                    else
                    {
                        off++;
                    }
                }
                mididata = fixed_mididata;
                fixed_len = midilen;
                SDL_Log("I_RegisterSong: Using fixed_mididata for FluidSynth");
            }
            else
            {
                SDL_Log("I_RegisterSong: SDL_malloc failed, using original mididata");
            }

            fluid_player_t *player = new_fluid_player(fluidsynth);
            if (!player)
            {
                SDL_Log("I_RegisterSong: Failed to create FluidSynth player");
                nomusic = true;
                return 0;
            }

            int add_result = fluid_player_add_mem(player, mididata, midilen);
            if (fixed_mididata)
            {
                SDL_free(fixed_mididata);
                fixed_mididata = nullptr;
            }
            if (add_result != FLUID_OK)
            {
                SDL_Log("I_RegisterSong: Failed to add MIDI to FluidSynth player");
                delete_fluid_player(player);
                nomusic = true;
                return 0;
            }

            // Store player for later use in I_PlaySong
            fluid_player = player;
            use_fluid_pcm = true;
            SDL_Log("I_RegisterSong: FluidSynth player created and MIDI loaded");
            return 1;
        }
        else
#endif
        {
            SDL_Log("I_RegisterSong: FluidSynth not available, music disabled");
            nomusic = true;
            return 0;
        }
    }
    SDL_Log("I_RegisterSong: Audio loaded successfully!");

#endif

    return 0;
}

void I_SetMusicVolume(int volume)
{
    // Both MIDI (FluidSynth) and OGG/other formats play through music_track,
    // so one gain call covers both.
#ifndef NO_MIXER
    if (nomusic)
        return;

    if (music_track)
    {
        float gain = (volume * 2) / 127.0f;
        MIX_SetTrackGain(music_track, gain);
    }
#endif
}

// Console variables for OpenAL
#ifdef ALAU
consvar_t cv_soundprovider = {"soundprovider", "auto", CV_SAVE, {"auto", "openal", "software", NULL}};
consvar_t cv_speakerconfig = {"speakerconfig", "stereo", CV_SAVE, {"mono", "stereo", "quadraphonic", "5.1", NULL}};
consvar_t cv_distance_model = {"distance_model", "inverse_clamped", CV_SAVE, {"none", "inverse", "inverse_clamped", "exponential", "exponential_clamped", NULL}};
consvar_t cv_dopplerfactor = {"doppler_factor", "1.0", CV_SAVE, CV_Float};
#endif

// Note: Additional console variable registration would be needed
// in SoundSystem::Startup() for these to be functional

// CD audio stubs - CD audio support was removed (i_cdmus.cpp deleted)
// These are stubs to satisfy linker dependencies
void I_InitCD() {}
void I_StopCD() {}
void I_PauseCD() {}
void I_ResumeCD() {}
void I_ShutdownCD() {}
void I_UpdateCD() {}
void I_PlayCD(int track, bool looping) {(void)track; (void)looping;}
int I_SetVolumeCD(int volume) {(void)volume; return 0;}
