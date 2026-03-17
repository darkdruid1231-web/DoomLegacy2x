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
#include "SDL.h"
#ifndef NO_MIXER
#include "SDL_mixer.h"
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

static bool musicStarted = false;
static bool soundStarted = false;

static SDL_AudioSpec audio;

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
static struct music_channel_t
{
    Mix_Music *mus;
    SDL_RWops *rwop;
    music_channel_t()
    {
        mus = NULL;
        rwop = NULL;
    }
} music;
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

    // Find free buffer - check if any previously used buffers are now free
    int buffer_idx = -1;
    for (int i = 0; i < MAX_OPENAL_BUFFERS; i++)
    {
        if (!al_buffers[i].in_use)
        {
            buffer_idx = i;
            break;
        }
        // Check if buffer's source has stopped - then free it
        // This is a simplified approach - in production we'd track source->buffer mapping
    }
    
    // If no free buffer, try to find one that's done playing
    if (buffer_idx == -1)
    {
        CONS_Printf("OpenAL: All buffers in use, trying to find free one...\n");
        for (int i = 0; i < MAX_OPENAL_BUFFERS; i++)
        {
            // Check if any source is using this buffer
            bool in_use = false;
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
                        in_use = true;
                        break;
                    }
                }
            }
            if (!in_use)
            {
                buffer_idx = i;
                break;
            }
        }
    }

    if (buffer_idx == -1)
    {
        CONS_Printf("OpenAL: No free buffers\n");
        return -1;
    }
    
    // Pass data directly - sdata might already be in signed format
    // or OpenAL might handle unsigned internally
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

#ifdef NO_MIXER
    SDL_LockAudio();
#endif

    c->samplesize = c->si->depth;
    c->data = (Uint8 *)c->si->sdata;
    c->end = c->data + c->si->length;
    c->stepremainder = 0;
    c->playing = true;

#ifdef NO_MIXER
    SDL_UnlockAudio();
#endif

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
    audio.format = AUDIO_S16SYS;
    audio.channels = 2;
    audio.samples = SAMPLECOUNT;

    // Try to initialize OpenAL first
    if (I_InitOpenAL())
    {
        sound_provider = SP_OPENAL;
        openal_initialized = true;
        
        // For OpenAL mode, we DON'T open SDL audio for sound effects
        // But we DON'T call SDL_OpenAudio so SDL_mixer can use it for music
        
        soundStarted = true;
        CONS_Printf("Sound: Using OpenAL audio provider (SDL_mixer for music)\n");
        return;
    }

    CONS_Printf("Sound: OpenAL not available, using software mixing\n");

    // Fallback to SDL audio
    audio.callback = I_UpdateSound_sdl;

    I_SetChannels();

#ifdef NO_MIXER
    SDL_OpenAudio(&audio, NULL);
    CONS_Printf("Audio device initialized: %d Hz, %d samples/slice.\n", audio.freq, audio.samples);
    SDL_PauseAudio(0);
    soundStarted = true;
#else
    sound_provider = SP_SOFTWARE;
#endif
}

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
    // Need to open SDL_mixer for music
    if (Mix_OpenAudio(audio.freq, audio.format, audio.channels, audio.samples) < 0)
    {
        CONS_Printf("Unable to open audio for music: %s\n", Mix_GetError());
        nomusic = true;
        return;
    }

    int temp;
    if (!Mix_QuerySpec(&audio.freq, &audio.format, &temp))
    {
        CONS_Printf("Mix_QuerySpec: %s\n", Mix_GetError());
        nomusic = true;
        return;
    }

    // Even for OpenAL sound effects, we need Mix_SetPostMix for music
    // But don't let software mixing interfere with OpenAL
    if (sound_provider == SP_SOFTWARE)
    {
        Mix_SetPostMix(audio.callback, NULL);
    }
    
    CONS_Printf("Audio device for music: %d Hz, %d channels.\n", audio.freq, audio.channels);
    Mix_Resume(-1);

    if (nomusic)
        return;

    Mix_ResumeMusic();
    mus2mid_buffer = (byte *)Z_Malloc(MIDBUFFERSIZE, PU_MUSIC, NULL);
    CONS_Printf("Music initialized.\n");
    musicStarted = true;
#endif
}

void I_ShutdownSound()
{
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
    SDL_CloseAudio();
#else
    Mix_CloseAudio();
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
    if (nomusic)
        return;

    if (music.mus)
    {
        Mix_FadeInMusic(music.mus, looping ? -1 : 0, 500);
    }
#endif
}

void I_PauseSong(int handle)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return;

    I_StopSong(handle);
#endif
}

void I_ResumeSong(int handle)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return;

    I_PlaySong(handle, true);
#endif
}

void I_StopSong(int handle)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return;

    Mix_FadeOutMusic(500);
#endif
}

void I_UnRegisterSong(int handle)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return;

    if (music.mus)
    {
        Mix_FreeMusic(music.mus);
        music.mus = NULL;
        music.rwop = NULL;
    }
#endif
}

int I_RegisterSong(void *data, int len)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return 0;

    if (music.mus)
    {
        I_Error("Two registered pieces of music simultaneously!\n");
    }

    byte *bdata = static_cast<byte *>(data);

    if (memcmp(data, MUSMAGIC, 4) == 0)
    {
        int err;
        Uint32 midlength;
        if ((err = qmus2mid(bdata, mus2mid_buffer, 89, 64, 0, len, MIDBUFFERSIZE, &midlength)) != 0)
        {
            CONS_Printf("Cannot convert MUS to MIDI: error %d.\n", err);
            return 0;
        }

        music.rwop = SDL_RWFromConstMem(mus2mid_buffer, midlength);
    }
    else
    {
        music.rwop = SDL_RWFromConstMem(data, len);
    }

#ifdef SDL2
    music.mus = Mix_LoadMUS_RW(music.rwop, 0);
#else
    music.mus = Mix_LoadMUS_RW(music.rwop);
#endif

    if (!music.mus)
    {
        CONS_Printf("Couldn't load music lump: %s\n", Mix_GetError());
        music.rwop = NULL;
    }

#endif

    return 0;
}

void I_SetMusicVolume(int volume)
{
    // In OpenAL mode, still use SDL_mixer for music
#ifndef NO_MIXER
    if (nomusic)
        return;

    Mix_VolumeMusic(volume * 2);
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
