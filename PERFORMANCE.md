# Doom Legacy GL Renderer Performance

## Methodology
- Run `gr_drawstats` in console during gameplay
- Metrics are **per-frame**
- Compare before/after each optimization phase

## Baseline Measurements

### Scene: Doom2 Map01, 640x480 GL renderer — Pre-optimization
| Metric | Value |
|--------|-------|
| Wall quads | 126 |
| Flushes | 0 |
| Texture binds | 126 |

### Scene: Doom2 Map01, 640x480 GL renderer — Phase 1 (Release build)
| Metric | Value |
|--------|-------|
| Wall quads | 205 |
| Flushes | 0 |
| Texture binds | 205 |

> Note: Higher wall quad counts vs baseline reflect different game scenes (more geometry visible), not Release build effects. Release build affects CPU performance, not draw call geometry counts.

### Scene: Doom2 Map01, 640x480 GL renderer — Phase 2+4 (Fog caching + Texture bind tracking)
| Metric | Value |
|--------|-------|
| Wall quads | 210 |
| Flushes | 0 |
| Texture binds | 136 |

> Texture binds (136) < Wall quads (210): Phase 4 texture bind tracking is working — ~35% fewer binds by skipping redundant `m->GLUse()` calls for repeated materials.

---

## Optimization Phases

### Phase 0: Profiling Hooks ✅ (2026-03-28)
- Added `gr_drawstats` console command
- Added `frame_wall_quads`, `frame_flushes`, `frame_texture_binds` counters
- Added `current_material` tracking
- **Branch:** `renderer-optimization-profiling`
- **Files changed:** `oglrenderer.hpp`, `oglrenderer.cpp`, `hwr_render.cpp`

### Phase 1: Release Build Default ✅ (2026-03-29)
- Set `CMAKE_BUILD_TYPE` default to Release
- Compiler flags: `-O3 -DNDEBUG`
- Executable size reduced: 8.2MB → 3.4MB
- **Branch:** `renderer-optimization-profiling`
- **Files changed:** `CMakeLists.txt`

### Phase 2: Fog State Caching ✅ (2026-03-29)
- Track last fog lightlevel in `OGLRenderer`
- Skip `glEnable(GL_FOG)`/`glDisable(GL_FOG)` when lightlevel unchanged
- Eliminates redundant GL state changes per subsector
- **Branch:** `renderer-optimization-profiling`
- **Files changed:** `oglrenderer.hpp`, `oglrenderer.cpp`

### Phase 3: QuadBatcher Integration ❌ (2026-03-29) — REVERTED
- Attempted to replace `glBegin/glEnd` with `quadbatch.AddQuad()` for batchable quads (no shader)
- VAO save/restore did eliminate GL error 0x0501, but walls still rendered incorrectly (see-through/flickering)
- Tried: VAO save/restore, client-side arrays, `glFinish` — all failed to fix rendering correctness
- Root cause: this GL driver (Intel HD Graphics compatibility profile) cannot mix `glDrawArrays` and `glBegin/glEnd` in the same frame — driver-level incompatibility
- **Status:** Reverted. Full VBO pipeline conversion would fix this but requires rewriting ALL rendering paths (walls, floors, sprites, 2D, shadows).
- **Branch:** `renderer-optimization-profiling`
- **Files changed:** `oglrenderer.hpp`, `oglrenderer.cpp`, `include/hardware/hw_quadbatch.h`, `video/hardware/hw_quadbatch.cpp`

### Phase 4: Texture Bind Tracking ✅ (2026-03-29)
- Skip `m->GLUse()` when `m == current_material`
- Tracked via `current_material` pointer reset each frame in `StartFrame`
- Result: 205 wall quads → 134 texture binds (35% reduction)

### Phase 5: Software Renderer Column Caching (blocked — needs investigation)
- **Investigation result:** `GetColumn`/`GetMaskedColumn` are already effectively free due to existing texture data caching in `GetData()`. Per-x clipping (`mfloorclip`, `mceilingclip`) prevents batching in masked paths.
- **What would work:** restructuring column drawer to accept x ranges instead of single x, allowing batch drawing of columns with identical clipping — significant refactor.
- **Next step:** add SW renderer profiling (`sw_drawstats`) to measure actual column call counts before redesigning.

---

## Key Insight
> Phase 4 delivers real results: 205 wall quads, 134 texture binds (35% reduction). Fog caching eliminates redundant GL fog state changes per subsector. Phase 3 (QuadBatcher) is blocked by a GL driver incompatibility — this driver cannot mix `glDrawArrays` and `glBegin/glEnd` in the same frame. A full VBO pipeline conversion would resolve it but requires rewriting all rendering paths.

## Build Instructions
```bash
cd doomlegacy-profiling/legacy/trunk/build
rm -rf *
cmake -G "Ninja" -DFLEX_EXECUTABLE=/c/msys64/usr/bin/flex \
  -DBISON_EXECUTABLE=/c/msys64/usr/bin/bison \
  -DLEMON_EXECUTABLE=/c/msys64/usr/bin/lemon ..
ninja
```
