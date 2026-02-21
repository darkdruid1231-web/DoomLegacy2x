# Doom Legacy Include Reorganization Plan

## Overview

The Doom Legacy project currently has **109 header files** in a flat `include/` directory, plus 8 files in `include/hardware/`. This plan reorganizes them into logical categories to improve code maintainability, discoverability, and reduce coupling.

---

## 1. Current Include Files and Their Purposes

### Core/Base Types (Fundamental definitions)
| File | Purpose |
|------|---------|
| `doomdef.h` | Common defines, version info, basic types |
| `doomtype.h` | Platform-dependent typedefs (uint8, int32, etc.) |
| `tables.h` | Lookup tables (angle to sine/cosine, etc.) |
| `m_fixed.h` | Fixed-point arithmetic functions |
| `vect.h` | Vector math (2D/3D) |
| `z_zone.h` | Zone memory allocator |
| `z_cache.h` | Cached memory management |
| `m_swap.h` | Byte swapping utilities |
| `m_bbox.h` | Bounding box utilities |

### Doom Data Structures (Game data formats)
| File | Purpose |
|------|---------|
| `doomdata.h` | Raw Doom data structures (maps, things) |
| `d_items.h` | Item definitions |
| `d_event.h` | Event types and structures |
| `d_ticcmd.h` | Tic command (player input) |
| `dstrings.h` | String tables (English/French) |
| `d_french.h` | French localization strings |
| `d_debug.h` | Debug definitions |
| `info.h` | Monster/weapon info tables (110KB, largest file) |
| `savegame.h` | Save/load game structures |

### Game Logic (Game state and entities)
| File | Purpose |
|------|---------|
| `g_game.h` | Game state management |
| `g_player.h` | Player class |
| `g_actor.h` | Actor/base entity class |
| `g_pawn.h` | Pawn (in-game entity) class |
| `g_map.h` | Map management |
| `g_level.h` | Level state |
| `g_think.h` | Thinker system (game objects) |
| `g_type.h` | Object type definitions |
| `g_team.h` | Team play |
| `g_decorate.h` | Decorate parser (DECORATE format) |
| `g_damage.h` | Damage system |
| `g_blockmap.h` | Blockmap collision |
| `g_input.h` | Input handling |
| `g_mapinfo.h` | Map info parsing (GWA/UMAPINFO) |

### Rendering (2D/3D graphics)
| File | Purpose |
|------|---------|
| `r_main.h` | Renderer main state, lighting |
| `r_defs.h` | Renderer definitions, structures |
| `r_data.h` | Texture/flat data management |
| `r_draw.h` | Software drawing routines |
| `r_bsp.h` | BSP tree handling |
| `r_plane.h` | Plane rendering |
| `r_segs.h` | Line segment rendering |
| `r_sprite.h` | Sprite rendering |
| `r_things.h` | Thing rendering |
| `r_sky.h` | Sky rendering |
| `r_splats.h` | Splats rendering |
| `r_render.h` | Render interface |
| `r_presentation.h` | Presentation layer |
| `screen.h` | Screen management |
| `v_video.h` | Video utilities |
| `vfile.h` | Virtual file handling |

### Audio (Sound and music)
| File | Purpose |
|------|---------|
| `s_sound.h` | Sound system interface |
| `sounds.h` | Sound effect definitions |
| `i_sound.h` | System sound interface |
| `qmus2mid.h` | Music conversion (_quake to midi) |

### Network (Multiplayer networking)
| File | Purpose |
|------|---------|
| `n_connection.h` | Network connection handling |
| `n_interface.h` | Network interface |
| `lnet.h` | Legacy networking (TNL) |
| `i_net.h` | System network interface |

### Input/Video System (Platform I/O)
| File | Purpose |
|------|---------|
| `i_video.h` | Video system interface |
| `i_system.h` | System interface (timing, etc.) |
| `i_sound.h` | (also in audio, referenced here) |

### UI/HUD (User interface)
| File | Purpose |
|------|---------|
| `m_menu.h` | Menu system |
| `hud.h` | Heads-up display |
| `st_lib.h` | Status bar library |
| `wi_stuff.h` | WInner/intermission screen |
| `f_finale.h` | Finale sequence |
| `am_map.h` | Automap |

### Scripting/ACS (Scripting engine)
| File | Purpose |
|------|---------|
| `t_acs.h` | ACS script compiler |
| `t_parse.h` | Script parsing |
| `t_script.h` | Script runtime |
| `t_func.h` | Script functions |
| `t_spec.h` | Special script functions |
| `t_vari.h` | Script variables |

### Map/Playzone (Map processing)
| File | Purpose |
|------|---------|
| `p_spec.h` | Special linedefs/sectors |
| `p_setup.h` | Map setup |
| `p_saveg.h` | Save/restore game objects |
| `p_maputl.h` | Map utilities |
| `p_polyobj.h` | Polyobjects |
| `p_camera.h` | Camera handling |
| `p_effects.h` | Visual effects |
| `p_enemy.h` | Enemy AI |
| `p_fab.h` | Fabs (3D mode building blocks) |
| `p_hacks.h` | Game hacks/compatibility |
| `p_pspr.h` | Player sprite |
| `ntexture.h` | Texture handling |

### Bots/AI (Artificial intelligence)
| File | Purpose |
|------|---------|
| `b_bot.h` | Bot AI |
| `b_path.h` | Pathfinding |
| `a_functions.h` | Actor functions |
| `acbot.h` | ACS bot interface |

### Console/Commands (Developer tools)
| File | Purpose |
|------|---------|
| `command.h` | Console commands |
| `console.h` | Console system |
| `cvars.h` | Console variables |
| `keys.h` | Key bindings |

### Utils (Utility functions)
| File | Purpose |
|------|---------|
| `m_argv.h` | Command line argument parsing |
| `m_random.h` | Random number generation |
| `m_archive.h` | Serialization/archive |
| `m_dll.h` | Dynamic library loading |
| `m_cheat.h` | Cheat code handling |
| `m_menu.h` | Menu utilities |
| `m_misc.h` | Miscellaneous utilities |
| `md5.h` | MD5 hashing |
| `filesrch.h` | File searching |
| `dictionary.h` | Dictionary/hashtable |
| `functors.h` | Functor objects |
| `parser.h` | Generic parser |
| `mnemonics.h` | Key mnemonics |
| `mserv.h` | Master server |
| `dehacked.h` | DeHackEd support |

### WAD/File Handling
| File | Purpose |
|------|---------|
| `wad.h` | WAD file handling |
| `w_wad.h` | WAD wrapper |
| `p5prof.h` | Profiling (p5) |

### Hardware/OpenGL (3D rendering - already in hardware/)
| File | Purpose |
|------|---------|
| `hardware/hwr_bsp.h` | OpenGL BSP |
| `hardware/hwr_geometry.h` | OpenGL geometry |
| `hardware/hwr_render.h` | OpenGL rendering |
| `hardware/hwr_states.h` | OpenGL state management |
| `hardware/md3.h` | MD3 model loading |
| `hardware/oglrenderer.hpp` | OpenGL renderer |
| `hardware/oglshaders.h` | OpenGL shaders |
| `hardware/oglhelpers.hpp` | OpenGL helpers |

---

## 2. Proposed Directory Structure

```
include/
├── core/                    # Core types and utilities
│   ├── doomdef.h
│   ├── doomtype.h
│   ├── tables.h
│   ├── m_fixed.h
│   ├── vect.h
│   ├── m_swap.h
│   ├── m_bbox.h
│   └── CMakeLists.txt
│
├── data/                    # Game data structures
│   ├── doomdata.h
│   ├── d_items.h
│   ├── d_event.h
│   ├── d_ticcmd.h
│   ├── dstrings.h
│   ├── d_french.h
│   ├── d_debug.h
│   ├── info.h
│   ├── savegame.h
│   └── CMakeLists.txt
│
├── game/                    # Game logic
│   ├── g_game.h
│   ├── g_player.h
│   ├── g_actor.h
│   ├── g_pawn.h
│   ├── g_map.h
│   ├── g_level.h
│   ├── g_think.h
│   ├── g_type.h
│   ├── g_team.h
│   ├── g_decorate.h
│   ├── g_damage.h
│   ├── g_blockmap.h
│   ├── g_input.h
│   ├── g_mapinfo.h
│   └── CMakeLists.txt
│
├── rendering/               # Software renderer
│   ├── r_main.h
│   ├── r_defs.h
│   ├── r_data.h
│   ├── r_draw.h
│   ├── r_bsp.h
│   ├── r_plane.h
│   ├── r_segs.h
│   ├── r_sprite.h
│   ├── r_things.h
│   ├── r_sky.h
│   ├── r_splats.h
│   ├── r_render.h
│   ├── r_presentation.h
│   ├── screen.h
│   ├── v_video.h
│   ├── vfile.h
│   └── CMakeLists.txt
│
├── audio/                   # Audio system
│   ├── s_sound.h
│   ├── sounds.h
│   ├── i_sound.h
│   ├── qmus2mid.h
│   └── CMakeLists.txt
│
├── network/                 # Networking
│   ├── n_connection.h
│   ├── n_interface.h
│   ├── lnet.h
│   ├── i_net.h
│   └── CMakeLists.txt
│
├── platform/                # Platform I/O
│   ├── i_video.h
│   ├── i_system.h
│   └── CMakeLists.txt
│
├── ui/                      # User interface
│   ├── m_menu.h
│   ├── hud.h
│   ├── st_lib.h
│   ├── wi_stuff.h
│   ├── f_finale.h
│   ├── am_map.h
│   └── CMakeLists.txt
│
├── scripting/               # Scripting engine
│   ├── t_acs.h
│   ├── t_parse.h
│   ├── t_script.h
│   ├── t_func.h
│   ├── t_spec.h
│   ├── t_vari.h
│   └── CMakeLists.txt
│
├── map/                     # Map processing
│   ├── p_spec.h
│   ├── p_setup.h
│   ├── p_saveg.h
│   ├── p_maputl.h
│   ├── p_polyobj.h
│   ├── p_camera.h
│   ├── p_effects.h
│   ├── p_enemy.h
│   ├── p_fab.h
│   ├── p_hacks.h
│   ├── p_pspr.h
│   ├── ntexture.h
│   └── CMakeLists.txt
│
├── ai/                      # AI and bots
│   ├── b_bot.h
│   ├── b_path.h
│   ├── a_functions.h
│   ├── acbot.h
│   └── CMakeLists.txt
│
├── console/                # Console/commands
│   ├── command.h
│   ├── console.h
│   ├── cvars.h
│   ├── keys.h
│   └── CMakeLists.txt
│
├── util/                    # Utilities
│   ├── m_argv.h
│   ├── m_random.h
│   ├── m_archive.h
│   ├── m_dll.h
│   ├── m_cheat.h
│   ├── m_misc.h
│   ├── md5.h
│   ├── filesrch.h
│   ├── dictionary.h
│   ├── functors.h
│   ├── parser.h
│   ├── mnemonics.h
│   ├── mserv.h
│   ├── dehacked.h
│   ├── p5prof.h
│   └── CMakeLists.txt
│
├── wad/                     # WAD/file handling
│   ├── wad.h
│   ├── w_wad.h
│   └── CMakeLists.txt
│
└── hardware/               # OpenGL/hardware renderer (existing)
    ├── hwr_bsp.h
    ├── hwr_geometry.h
    ├── hwr_render.h
    ├── hwr_states.h
    ├── md3.h
    ├── oglrenderer.hpp
    ├── oglshaders.h
    ├── oglhelpers.hpp
    └── CMakeLists.txt
```

---

## 3. Compatibility Strategy

### Option A: Include Prefixes (Recommended)

Maintain backward compatibility by keeping wrapper headers in the original location that include the new location:

**Example:** `include/doomdef.h` becomes a thin wrapper:
```c
// Backward compatibility wrapper - include core/doomdef.h instead
#ifndef doomdef_h
#define doomdef_h
#include "core/doomdef.h"
#endif
```

**Pros:**
- Zero external changes required
- Gradual migration path
- Clear deprecation path

**Cons:**
- Extra file maintenance
- Slight compile time overhead (negligible)

### Option B: CMake Include Path Manipulation

Update CMake to add new directories AND keep old:
```cmake
# Add new structure
include_directories(${CMAKE_SOURCE_DIR}/include/core)
include_directories(${CMAKE_SOURCE_DIR}/include/game)
# ... etc

# Keep old path for compatibility (deprecated)
include_directories(${CMAKE_SOURCE_DIR}/include)
```

### Option C: Combined Approach (Best)

Use both:
1. Move files to new structure
2. Create compatibility wrappers in old locations
3. Update CMake to prefer new paths
4. Add deprecation warnings to wrappers

---

## 4. Step-by-Step Migration

### Phase 1: Preparation (Week 1)
1. Create all new subdirectories
2. Add CMakeLists.txt to each new directory
3. Create backward-compatibility wrappers for each moved file
4. Update main CMakeLists.txt to include new paths

### Phase 2: Core & Data (Week 1-2)
1. Move `core/` files (doomdef.h, doomtype.h, m_fixed.h, etc.)
2. Move `data/` files (doomdata.h, d_*.h)
3. Update references in source files
4. Test compilation

### Phase 3: Major Systems (Week 2-3)
1. Move `game/` files
2. Move `rendering/` files  
3. Move `audio/` files
4. Move `network/` files

### Phase 4: Supporting Systems (Week 3-4)
1. Move `platform/` files
2. Move `ui/` files
3. Move `scripting/` files
4. Move `map/` files

### Phase 5: Remaining Systems (Week 4-5)
1. Move `ai/` files
2. Move `console/` files
3. Move `util/` files
4. Move `wad/` files
5. Keep `hardware/` as-is (already organized)

### Phase 6: Cleanup (Week 5-6)
1. Remove backward-compatibility wrappers one by one
2. Update all `#include` statements to use new paths
3. Remove old include path from CMake
4. Update documentation

---

## 5. CMake Updates Needed

### Current CMake (line 183)
```cmake
include_directories(include)
```

### Updated CMake Structure

```cmake
# New include structure
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/core)
include_directories(${CMAKE_SOURCE_DIR}/include/data)
include_directories(${CMAKE_SOURCE_DIR}/include/game)
include_directories(${CMAKE_SOURCE_DIR}/include/rendering)
include_directories(${CMAKE_SOURCE_DIR}/include/audio)
include_directories(${CMAKE_SOURCE_DIR}/include/network)
include_directories(${CMAKE_SOURCE_DIR}/include/platform)
include_directories(${CMAKE_SOURCE_DIR}/include/ui)
include_directories(${CMAKE_SOURCE_DIR}/include/scripting)
include_directories(${CMAKE_SOURCE_DIR}/include/map)
include_directories(${CMAKE_SOURCE_DIR}/include/ai)
include_directories(${CMAKE_SOURCE_DIR}/include/console)
include_directories(${CMAKE_SOURCE_DIR}/include/util)
include_directories(${CMAKE_SOURCE_DIR}/include/wad)
include_directories(${CMAKE_SOURCE_DIR}/include/hardware)
include_directories(tnl)
```

### Per-Directory CMakeLists.txt

Example for `include/core/CMakeLists.txt`:
```cmake
# Core headers - used by everything
set(CORE_HEADERS
    doomdef.h
    doomtype.h
    tables.h
    m_fixed.h
    vect.h
    m_swap.h
    m_bbox.h
)

add_library(core INTERFACE)
target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
```

### Alternative: Use Generator Expressions

```cmake
# More modern CMake approach
foreach(subdir IN ITEMS core data game rendering audio network platform ui scripting map ai util wad hardware)
    add_library(include_${subdir} INTERFACE)
    target_include_directories(include_${subdir} INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${subdir}>
        $<INSTALL_INTERFACE:include/${subdir}>
    )
endforeach()
```

---

## 6. Migration Script Example

A Python script to automate the migration:

```python
#!/usr/bin/env python3
"""Migrate includes to new directory structure."""

import os
import shutil
from pathlib import Path

# Mapping of files to new directories
MIGRATION_MAP = {
    # Core
    'doomdef.h': 'core',
    'doomtype.h': 'core',
    'tables.h': 'core',
    'm_fixed.h': 'core',
    'vect.h': 'core',
    'm_swap.h': 'core',
    'm_bbox.h': 'core',
    # Data
    'doomdata.h': 'data',
    'd_items.h': 'data',
    'd_event.h': 'data',
    # ... etc
}

def migrate(include_dir):
    for filename, subdir in MIGRATION_MAP.items():
        src = include_dir / filename
        dst = include_dir / subdir / filename
        
        if src.exists():
            # Move file
            shutil.move(str(src), str(dst))
            # Create compatibility wrapper
            create_compat_wrapper(include_dir, filename, subdir)

def create_compat_wrapper(include_dir, filename, subdir):
    wrapper = include_dir / filename
    wrapper.write_text(f'''// Backward compatibility - use {subdir}/{filename}
#pragma message("Include {subdir}/{filename} instead of {filename}")
#include "{subdir}/{filename}"
''')
```

---

## 7. Estimated Impact

| Metric | Current | After |
|--------|---------|-------|
| Files per directory | 109 | ~10-15 avg |
| Max depth | 1 | 2 |
| Related files co-located | No | Yes |
| Build system clarity | Low | High |

---

## 8. Open Questions

1. **Deprecation timeline?** - Keep old paths for 1 release? 2?
2. **Breaking changes?** - When to remove backward compatibility?
3. **Documentation updates?** - Update all code comments referencing old paths?
4. **CI/CD updates?** - Ensure CI uses new structure from day 1

---

## Summary

This reorganization will:
- **Reduce cognitive load** by grouping related headers
- **Improve maintainability** with clear module boundaries  
- **Enable modular builds** with per-directory CMake
- **Preserve compatibility** via include aliases during transition
- **Future-proof** the codebase for new features

The migration can be done incrementally over 5-6 weeks with minimal risk using the backward-compatibility strategy.
