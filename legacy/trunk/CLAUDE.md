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

## SW Renderer Notes

- **Sprite thinker loop**: `r_main.cpp` runs a thinker-loop pass over all actors after BSP
  traversal as a safety net. The validcount guard (`if sec->validcount == validcount continue`)
  was removed — the loop now unconditionally re-projects all actors. Double-projection of opaque
  sprites is harmless; removing the guard fixed invisible moving-monster sprites.
- **`spritepres_t::Project()` guard**: Before accessing `mat->tex[0]`, always check
  `if (!mat || mat->tex.empty() || !mat->tex[0].t || mat->worldwidth <= 0) return`. Walking
  frames with unloaded rotation slots produce dummy Materials with empty tex[].
- **`spritelights`**: Exported as `extern` in `include/r_things.h` so `r_main.cpp` can set it
  per-actor before calling Project().

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
- **hw_gbuffer**: G-buffer abstraction in `video/hardware/hw_gbuffer.cpp` — normals
  (attachment 1) and albedo (attachment 2) hang off `scene_fbo` (PostFX owns the MRT FBO).
  Never share textures between FBOs — Windows drivers skip render-cache flushes on transitions
  between FBOs that share texture objects (caused a 30s black-wall glitch).
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

## GL Advanced Rendering (PostFX / Deferred)

Phase-numbered improvements tracked in source comments. Active as of 2026-03:

| Phase | Feature | Key files |
|-------|---------|-----------|
| 2.1 | HDR tone-mapping | `hw_postfx.cpp` |
| 2.2 | G-buffer MRT FBO — normals for SSAO | `hw_gbuffer.h/.cpp` |
| 2.3 | Single-light depth shadow map | `hw_shadowmap.h/.cpp` |
| 3.1 | Deferred dynamic light accumulation | `hw_gbuffer.h/.cpp`, `oglshaders.cpp` |
| 3.1b | Pre-fog albedo G-buffer attachment (attachment 2) | `hw_gbuffer.h/.cpp`, `hw_postfx.h/.cpp` |
| 3.3 | SSAO correctness (proper projection, host-shader gNormal output) | `oglshaders.cpp`, `hw_postfx.cpp` |
| 4.1 | Screen-space reflections (SSR) | `hw_postfx.h/.cpp` |
| 4.2 | Volumetric fog PostFX pass | `hw_postfx.h/.cpp` |
| 5.1 | Blinn-Phong specular in deferred pass | `hw_gbuffer.h/.cpp` |

Key architectural notes:
- `scene_fbo` (PostFX) is the MRT FBO: attachment 0 = color, 1 = normals, 2 = albedo.
  `BindGBuffer()` enables [0,1,2]; normal 3D rendering uses [0] only.
- `light_accum_fbo` / `light_accum_tex` (DeferredRenderer, RGBA16F) is completely independent —
  shares NO textures with scene_fbo, ensuring driver render-cache flushes on every transition.
- Deferred rendering gated on `cv_grdeferred.value`. Set `gr_fog 0` when `gr_volfog 1` to
  avoid double-fogging.
- The `s_null_fog_tex` (1×1 transparent) fallback prevents black-screen when vol-fog FBO is unbound.

## Widget System / MENUDEF

New component-based menu system alongside the classic `menuitem_t` flat-loop renderer:

- **`engine/menu/widget.h/.cpp`** — Widget ABC + concrete types (Label, Space, Slider, TextInput,
  Option, Button, List, Separator, Image) + WidgetPanel + MenuDrawCtx.
- **`engine/menu/menudef.h/.cpp`** — MENUDEF text-lump parser; `MenuDef_LoadAll()` called from
  `d_main.cpp` after mapinfo load. `DrawMenuModern()` checks `MenuDef_FindPanel(title)` first.
- `menuitem_t` is defined only inside `menu.cpp` (not exported); `MenuToWidgetPanel()` is a
  static function in `menu.cpp`.
- `s_widget_panels` (`std::map<Menu*, WidgetPanel*>`) in `menu.cpp` — panels built lazily on first
  `DrawMenuModern()` call.
- `enabled_if "cvar_name"` MENUDEF syntax grays/disables widgets when that cvar == 0.

## PK3 / Asset Loading

ZDoom-compatible PK3 namespace processing in `video/r_data.cpp` `ReadTextures()`:

| Path prefix | Treatment | Notes |
|---|---|---|
| `textures/`, `patches/`, `flats/`, `sprites/` | `h_start=false`, xscale=1 | create or replace |
| `hires/` (all sub-dirs) | `h_start=true` | replace existing, preserve world dims |
| `hires/sprites/` | `h_start=true`, fallback to false | scale to original sprite dims |

**Key gotcha**: `hires/textures/` **must** use `h_start=true` (PK3_HIRES), not PK3_TEXTURES —
otherwise the HD texture renders at xscale=1 and appears too large for walls.

`filter/GAME/` prefix stripping is applied so `filter/doom/hires/textures/WALL01.png` resolves
to `hires/textures/WALL01.png`.

## Sound System

- `soundcache_t` is a class defined **only inside** `audio/s_sound.cpp` — not in any header.
  External code must use `S_PrecacheSound(const char *name)` (declared in `s_sound.h`).
  Never call `soundCache.Get(...)` from outside `s_sound.cpp` — it won't compile.

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
