# Doom Legacy

A source port of the classic Doom engine, extended with OpenGL rendering, multiplayer, splitscreen, and mod support. The codebase is a mix of C and C++ targeting the `legacy/trunk/` directory.

## Features

Full feature list: [`docs/features.md`](legacy/trunk/docs/features.md)

### Graphics
- OpenGL hardware-accelerated renderer ✅
- Dynamic GL node generation (ZDBSP) - 🚧
- Deferred lighting with G-buffer (normals, albedo, depth)
- Dynamic lights: per-actor light definitions (LIGHTDEFS lump), projectile lights, weapon flash
- Corona halos with distance fade and projectile flicker
- Blob shadows (Doom64-style), bloom, SSAO, screen-space reflections, volumetric fog 🚧
- MD2/MD3 model support ✅
- glb model support (future) ❌
- Hi-res PNG/JPEG texture and sprite support (tested with DoomHDTextures.pk3 & marcelus_hd_sprites.pk3)
- Software renderer fallback ❌ - Likely broken right now

### Gameplay
- Doom, Heretic, and Hexen support
- Up to 4-player splitscreen 🚧
- Deathmatch, co-op, (CTF, domination, assault in future)
- Level hubs in future (return to visited maps)

### Editing / Modding
- WAD, PK3/ZIP, PAK, and directory resource files ✅
- ZDoom-style MAPINFO, partial DECORATE, SNDINFO, SNDSEQ, ANIMDEFS ✅
- DeHackEd / BEX support 🚧
- FraggleScript scripting
- LIGHTDEFS lump for data-driven per-actor light definitions ✅

## Building

Full compile instructions: [`docs/compiling.md`](legacy/trunk/docs/compiling.md)

The CI pipeline builds Windows only using MSYS2 MinGW64 with Ninja — this is the supported and tested build path.

### Windows (MSYS2/MinGW64)

1. Install [MSYS2](https://www.msys2.org/) and open the **MSYS2 MinGW 64-bit** shell.
2. Install dependencies:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-ninja mingw-w64-x86_64-cmake \
             mingw-w64-x86_64-pkg-config mingw-w64-x86_64-glew \
             mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer \
             mingw-w64-x86_64-libpng mingw-w64-x86_64-libjpeg-turbo \
             mingw-w64-x86_64-openal mingw-w64-x86_64-libvorbis \
             mingw-w64-x86_64-flac mingw-w64-x86_64-mpg123 \
             mingw-w64-x86_64-opus mingw-w64-x86_64-wavpack \
             mingw-w64-x86_64-zlib mingw-w64-x86_64-freetype \
             mingw-w64-x86_64-enet flex lemon bison
   ```
3. Build:
   ```bash
   cd legacy/trunk
   mkdir -p build && cd build
   cmake -G Ninja ..
   ninja
   ```

> **Important:** Always use the Ninja generator. Do **not** use `-G "MinGW Makefiles"` — its incremental dependency tracker is broken on Windows and will silently skip recompilation of changed files.

Incremental builds: just run `ninja` again from the `build/` directory.

## Running

```bash
# OpenGL mode (recommended)
./doomlegacy -opengl -iwad doom.wad

# Software mode
./doomlegacy -iwad doom.wad

# Jump straight to a level
./doomlegacy -opengl -iwad doom.wad -warp 1 1

# Play back a demo
./doomlegacy -iwad doom.wad -playdemo demo1
```

A Doom IWAD is required (`doom.wad`, `doom2.wad`, `doom1.wad`, etc.).

### In-game console

Press the key below **Esc** to open the console. Full command reference: [`docs/console.md`](legacy/trunk/docs/console.md)

Useful OpenGL cvars:

| CVar | Default | Description |
|---|---|---|
| `gr_dynamiclighting` | On | Enable per-actor dynamic lights |
| `gr_coronas` | On | Enable corona halos on light sources |
| `gr_coronasize` | 1 | Corona scale (1–10) |
| `gr_shadows` | On | Enable blob shadows |
| `gr_staticlighting` | On | Enable static light emitters |
| `gr_deferred` | Off | Deferred G-buffer lighting pass |

## Project Structure

```
legacy/trunk/
  engine/       Game logic: AI, actors, physics, map loading
  audio/        SDL_mixer sound and music
  video/        Rendering: OpenGL renderer, software renderer, hardware/
    hardware/   GL-specific: oglrenderer, shaders, G-buffer, shadow maps, post-FX
  net/          Networking (TNL + LNet)
  interface/    SDL input/video abstraction
  util/         Static utility library
  include/      All public headers
  resources/    legacy.wad assets (textures, LIGHTDEFS, lumps)
  docs/         HTML documentation
  tests/        Unit and integration tests
```

Key headers:
- `include/doomdef.h` — core constants and enums
- `include/doomtype.h` — core types (`fixed_t`, `angle_t`)
- `include/hardware/oglrenderer.hpp` — OpenGL renderer class
- `include/hardware/hw_gbuffer.h` — deferred G-buffer
- `include/hardware/hw_lightdefs.h` — LIGHTDEFS data-driven lights
- `include/g_actor.h` — base actor class

## Testing

```bash
cd legacy/trunk/build
ctest              # run all registered tests
./test_fixed_t     # fixed-point arithmetic
./test_actor       # actor/mobj system
./test_console     # console output
./test_mapinfo     # MAPINFO parsing
./test_serialization  # save/load serialization
```

Some tests require a WAD file in the build directory — copy or symlink `doom.wad` or `doom2.wad` there first.

### Smoke test

```bash
./scripts/smoke_test.sh
```

Validates the binary exists and responds to `--help`.

## Documentation

All docs are in [`legacy/trunk/docs/`](legacy/trunk/docs/):

| File | Contents |
|---|---|
| `features.md` | Full feature list and implementation status |
| `compiling.md` | Detailed build instructions for all platforms |
| `console.md` | All console commands and cvars |
| `editing.md` | WAD/mod editing reference |
| `legacy.md` | General gameplay manual |
| `TODO.md` | Known issues and planned work |
| `BUGS.md` | Known bugs |

## License

Based on the Doom Legacy engine. Licensed under the GNU General Public License v2 or later. See source file headers for details.

## Credits

- Original Doom engine: id Software
- Doom Legacy development team
- See [`docs/legacy.html`](legacy/trunk/docs/legacy.html) for full credits
