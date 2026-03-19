# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Doom Legacy is a Doom/Doom II source port - a classic first-person shooter game engine. The codebase is a mix of C and C++ with a CMake build system.

## Building the Project

### Windows (MSYS2/MinGW64)

```bash
# Open MSYS2 MinGW64 shell
cd legacy/trunk
mkdir build && cd build
cmake .. -G Ninja
ninja
```

Incremental builds: just run `ninja` again — it correctly detects changed files.

**Do not use `cmake .. -G "MinGW Makefiles"` or `mingw32-make` directly** — the MinGW
Makefile generator has a broken `compiler_depend.make` on this machine and will silently
skip recompilation of changed files.

### Linux / MSYS2 POSIX

```bash
cd doomlegacy-svn/legacy/trunk
mkdir build && cd build
cmake .. -G Ninja
ninja
```

The build requires out-of-tree builds (CMake will error if you try in-tree).

### Running Tests

Some tests require a WAD file. Copy `doom.wad` or `doom2.wad` into the build
directory, or symlink from `../../legacy_one/trunk/wads/`.

```bash
cd build
cmake ..
make
ctest              # Run all tests
./test_fixed_t     # Run specific unit test
./test_actor       # Run actor tests
./test_console     # Run console tests
```

Tests are individual executables in the build directory. Key tests:
- `test_fixed_t` - Fixed-point arithmetic
- `test_actor` - Actor/mobj system
- `test_console` - Console output
- `test_vector` - Vector math
- `test_mapinfo` - Mapinfo parsing
- `test_serialization` - Save/load serialization
- `test_network_integration` - Network code
- `test_parity` - C++ vs C feature parity

### Smoke Test

```bash
./scripts/smoke_test.sh
```

Validates the binary exists and responds to `--help`.

## Architecture

### Core Components

- **engine/** - Game engine core: AI states, actor system, rendering, physics
- **audio/** - Sound and music (SDL_mixer)
- **video/** - Graphics rendering (OpenGL)
- **net/** - Network code (client/server)
- **interface/** - SDL input/video abstraction
- **util/** - Utility library (static)
- **lnet/** - Modern networking layer (ENet, GNS, TNL backends)
- **grammars/** - Lexer/parser generators (flex/lemon)
- **tnl/** - The Nethernet library (legacy networking)

### Key Classes

- `AActor` (g_actor.h) - Base object class (mobs, items, players)
- `APawn` (g_pawn.h) - Player-pawn (in-game representation)
- `player_t` / `player_s` (g_player.h) - Player state
- `g_game` (g_game.h) - Main game loop and state
- `DNet` (net/) - Network protocol handler

### Data Types

- `fixed_t` (doomtype.h) - 32-bit fixed-point (16.16 format)
- `angle_t` - 32-bit angle (0-0xFFFFFFFF)
- `mobjtype_t` - Enumeration for mobj types (MT_*)

### Network Architecture

The project has dual networking stacks:
1. **TNL** (tnl/) - Original networking with RPC and ghost replication
2. **LNet** (lnet/) - Modern callback-based networking with multiple backends (ENet, GNS)

## Important Paths

- `include/` - All public headers
- `include/doomdef.h` - Core definitions (constants, enums)
- `include/doomtype.h` - Core types (fixed_t, angle_t)
- `engine/ai_states.cpp` - Monster AI state machine (massive file)
- `engine/ai_mobjinfo.cpp` - Monster definitions
- `tests/` - Test suite (unit/ and integration/)

## 2D Rendering Pipeline

### Coordinate systems
- **OGL 2D space**: `Setup2DMode()` calls `gluOrtho2D(0,1,0,1)` then applies a HUD aspect-ratio
  transform (translate + scale for letterboxing/pillarboxing). All `Draw2DGraphic` calls take
  coordinates in this [0,1]×[0,1] space — not screen pixels, not virtual 320×200 units.
- **Virtual (Doom) units**: `Draw2DGraphic_Doom` accepts coords in BASEVIDWIDTH×BASEVIDHEIGHT
  (320×200) virtual units and normalises to [0,1] internally.
- **`consolemode` flag**: `Draw2DGraphic` asserts `consolemode == true`. It is set by
  `Setup2DMode()`, which is called from `SetFullScreenViewport()` at the start of every frame.
  Drawing 2D outside the normal frame (e.g. from a custom render path) requires an explicit
  `Setup2DMode()` call first.

### `Material::Draw` with `V_SSIZE` flag
- OGL: normalises the image's `worldwidth` by `BASEVIDWIDTH` (320). A 1456-px-wide image
  produces r = 1456/320 ≈ 4.55 — way off screen. Only use `V_SSIZE` for images authored at
  320×200 virtual resolution.
- SW: scales draw size by `vid.dupx`/`vid.dupy` and clips to screen. Same overflow issue for
  large source images.
- Use `Material::DrawStretched(x, y, w, h)` instead when you need an image to fill an arbitrary
  pixel rectangle regardless of its native dimensions (added in `video/v_video.cpp`).

### Texture formats in memory
- `PNGTexture::GetData()` (SW path): returns **8-bit palettized, column-major** pixel data.
- `PNGTexture::GLGetData()` (OGL path): returns **RGBA row-major** pixel data.
- Changing which path is used requires a different `ReadData()` call — they are mutually exclusive.

### Loading screen / startup console
- The loading screen (`con.refresh == true`) runs while the game is still initialising, before
  `vid.SetMode()` is called.
- At this point `vid.width = BASEVIDWIDTH` (320) and `vid.height = BASEVIDHEIGHT` (200) —
  the default values set by `I_StartupGraphics` before any SDL window is created.
- `workinggl == false` during the loading screen; `oglrenderer->ReadyToDraw()` returns false.
  OGL draw calls silently no-op. The SW framebuffer is used instead, even in OGL mode.
- `con.refresh` is set to false in `d_main.cpp` just before `vid.SetMode()`, which is where the
  real resolution is applied and `workinggl` becomes true.
- The loading screen is also called the **loading screen** or **startup screen** in comments.

### CONSBACK console background
- Lump name `CONSBACK`, sourced from `legacy/trunk/resources/CON_background.png`, packed into
  `legacy.wad` via `legacy.wad.inventory`.
- In OGL mode the background is always drawn regardless of `con_backpic` cvar; in SW mode the
  cvar `con_backpic` toggles between the picture and the translucent overlay.

## GL / BSP Rendering Notes

Active development area. Key gotchas:
- **GL_VERT flag**: Vertices from the GL node builder use the `0x8000` prefix
  convention — mask with `& 0x7FFF` before indexing into `vertexes[]`
- **Degenerate subsectors**: Subsectors with near-zero area must be skipped
  in the renderer (already guarded in `gl_bsp.cpp`)
- **glvertex allocation**: The GL vertex count is separate from the map vertex
  count. Don't confuse `numvertexes` with `numglvertexes`.
- **zdbsp_integration**: New wrapper (`engine/zdbsp_integration.cpp`) around
  the FNodeBuilder class for in-process GL node generation
- **hw_gbuffer**: New G-buffer abstraction in `video/hardware/hw_gbuffer.cpp`
- **WAD GL nodes pointer aliasing**: When GL nodes come from the WAD (`gllump != -1`),
  `mp->nodes == mp->glnodes`, `mp->segs == mp->glsegs`, `mp->subsectors == mp->glsubsectors`
  (same allocation). However `glvertexes` and `vertexes` stay **separate** — linedef v1/v2
  still point into `vertexes[]`, not `glvertexes[]`. The dynamic-build path merges them; the
  WAD path does not.
- **Player start spawn timing**: Player start mapthing_t objects are initialized with `tid=-100`
  so all 4 split-screen players can spawn immediately at maptic=0. `CheckRespawnSpot` blocks
  reuse for `PLAYERSPAWN_HALF_DELAY` (10) ticks after each spawn.
- **RenderGLSubsector bounds check**: Use `>=` not `>` when checking subsector index against
  `mp->numglsubsectors`. The fixed check is `if (num < 0 || num >= mp->numglsubsectors)`.
  The off-by-one (`>` instead of `>=`) caused a crash in 4-player split-screen (fixed 2026-03-15).

## Dependencies

- SDL + SDL_mixer
- OpenGL
- GLEW (OpenGL extension wrangler)
- PNG / JPEG
- TNL (The Nethernet)
- ENet (optional, for lnet)
- flex + lemon (parser generators)

## Notes

- The codebase uses both C and C++ (mix of .c and .cpp files)
- Parser files in grammars/ are generated by flex/lemon
- Some tests require a WAD file (e.g., DOOM2.WAD) in the build directory
- The project targets C++11 (c++0x in older CMake)
