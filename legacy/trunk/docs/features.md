# Doom Legacy features

As of version 2.0, the Legacy game engine has been rewritten in C++. A host of features were added to the game. Here is a short summary.

**Status indicators:**
- *Planning stage*
- *Partly implemented*
- *Almost finished*
- Done

## Graphics

- *Almost finished* — A brand new OpenGL graphics engine.
- Support for v2 and v5 GL nodes and glVIS.
- Hi-res sprites/textures/flats, PNG and JPEG/JFIF support.
- Generic graphic presentations for game objects. Support for sprites, *MD2s and MD3s* (partly implemented).

## Sound and Music

The preferred multimedia interface is now SDL, the Simple DirectMedia Layer. This makes Legacy easily portable and offers great multimedia capabilities. Music is produced using the SDL_mixer library. Highlights:

- Supports MUS, MIDI, MP3 and Ogg Vorbis music, as well as various module formats.
- Original Doom and WAV/RIFF sounds.
- Software 3D sound renderer.

## Gameplay

Doom Legacy 2.0 supports Doom, Heretic and Hexen. Gameplay is nearly the same as in original games. However, it can be enhanced with the following features:

- *Partly implemented* — Completely new netcode using OpenTNL.
- Advanced teamplay. Deathmatch, co-operative, *capture the flag, domination, assault* (planning stage).
- Client-side bots (*as DLLs* — planning stage).
- Asynchronous map changes, several maps can be run simultaneously.
- Level hubs (it's possible to return to a previously visited map).
- All weapons from Doom/Heretic/Hexen usable in any game type if you supply the required graphics and sounds! Weapon groups à la Half-Life.
- Customizable translucent HUD.
- Up to four-way splitscreen enabling several local players.

## Editing

- LevelInfo lumps.
- Hexen/ZDoom-style MAPINFO support, with extensions. Free level ordering, hubs à la Hexen.
- Partial DECORATE support (Actors).
- SNDINFO and SNDSEQ support, with extensions.
- ANIMDEFS support.
- Supported datafile formats: WAD, WAD2/3, PAK, ZIP/PK3.
- Normal directories can be used as resource "files".
- Essential DeHackEd and BEX support with extensions.
- Cross-over games: Doom/Heretic/Hexen entities in the same level, players playing Hellspawn instead of Marines...
- Voodoo dolls finally work.
- Internal BLOCKMAP builder enables ridiculously huge maps.
- *DLL modifiability* (planning stage).

## Other new features of interest (mainly for programmers)

- Zone Memory system replaced with malloc/free. No more fiddling with -mb.
- Second level dedicated cache system for textures, sounds, sprites etc.
- New game objects with custom AI are easy to make through inheritance.
- New save/load system with zlib compression.
- Damage system with different types of damage.
- Beautiful Doxygen documentation of the source code.
