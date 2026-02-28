//-----------------------------------------------------------------------------
//
// Magic numbers and constants extracted from HTML documentation
// Applied to collision/BSP code for consistency and maintainability
//
// References:
// - boomref.html: Contains linedef type allocations and bitmasks
// - whatsnew.html: Contains engine-specific constants
//
//-----------------------------------------------------------------------------

#ifndef P_MAGIC_H
#define P_MAGIC_H

// ============================================================================
// Linedef type allocations from boomref.html
// These are shifted values for generalized linedef types
// ============================================================================

#define MAGIC_FLOOR_TYPES       0x6000      // Floor linedef type base
#define MAGIC_CEILING_TYPES     0x4000      // Ceiling linedef type base  
#define MAGIC_DOOR_TYPES        0x3c00      // Door linedef type base
#define MAGIC_LOCKED_TYPES      0x3800      // Locked door type base
#define MAGIC_LIFT_TYPES        0x3400      // Lift linedef type base
#define MAGIC_STAIR_TYPES       0x3000      // Stair linedef type base
#define MAGIC_CRUSHER_TYPES     0x2f80      // Crusher linedef type base

// Bit shifts for generalized linedef fields (from boomref.html)
#define MAGIC_TRIGGER_SHIFT     0           // Bits 0-2: trigger type
#define MAGIC_SPEED_SHIFT       3           // Bits 3-4: speed 
#define MAGIC_MODEL_SHIFT       5           // Bit 5: model
#define MAGIC_DIRECT_SHIFT      6           // Bit 6: direction
#define MAGIC_TARGET_SHIFT      7           // Bits 7-9: target
#define MAGIC_CHANGE_SHIFT      10          // Bits 10-11: change
#define MAGIC_CRUSH_SHIFT       12          // Bit 12: crush

// Bitmasks for generalized linedef fields
#define MAGIC_TRIGGER_MASK      0x0007      // 3 bits
#define MAGIC_SPEED_MASK        0x0018      // 2 bits
#define MAGIC_MODEL_MASK        0x0020      // 1 bit
#define MAGIC_DIRECT_MASK       0x0040      // 1 bit
#define MAGIC_TARGET_MASK       0x0380      // 3 bits
#define MAGIC_CHANGE_MASK       0x0c00      // 2 bits
#define MAGIC_CRUSH_MASK        0x1000      // 1 bit

// ============================================================================
// Sector special flags
// ============================================================================

#define MAGIC_SECTOR_PUSHER     0x200       // Bit 9: sector has pusher
#define MAGIC_SECTOR_PUSH_MASK  0x200       // Same as above (for clarity)

// ============================================================================
// Blockmap constants (from p_local.h)
// ============================================================================

#define MAGIC_BLOCKMAP_UNITS    128         // MAPBLOCKUNITS
#define MAGIC_BLOCKMAP_SIZE     (128*FRACUNIT)  // MAPBLOCKSIZE
#define MAGIC_BLOCKMAP_SHIFT    (FRACBITS+7)    // MAPBLOCKSHIFT
#define MAGIC_BLOCKMAP_MASK     (128*FRACUNIT-1) // MAPBMASK
#define MAGIC_BLOCK_TO_FRAC     (MAGIC_BLOCKMAP_SHIFT-FRACBITS) // MAPBTOFRAC

// ============================================================================
// Collision detection constants
// ============================================================================

#define MAGIC_USERANGE          64          // Distance for use/pickup
#define MAGIC_MELEERANGE        64          // Melee attack range
#define MAGIC_MISSILERANGE      (32*64)     // Missile attack range
#define MAGIC_MAX_RADIUS        32          // Max thing radius for collisions
#define MAGIC_MAX_STEPMOVE      24          // Max step up/down without jumping
#define MAGIC_MAX_MOVE          (30/NEWTICRATERATIO)  // Max thing movement per tic
#define MAGIC_VIEWHEIGHT        41          // Player view height

// ============================================================================
// Intercept constants
// ============================================================================

#define MAGIC_INTERCEPT_INITIAL 128         // Initial intercepts allocation
#define MAGIC_INTERCEPT_GROW    2           // Growth factor for realloc

// ============================================================================
// Angle conversion constants
// ============================================================================

#define MAGIC_ANGLE_FINE        8192        // FINEMASK + 1
#define MAGIC_ANGLE_HALF        1024         // Half-angle for fine tangents

// ============================================================================
// FPS and timing constants (from doomstat.h reference)
// ============================================================================

#define MAGIC_TIC_RATE          35           // Default tic rate (DOOM)
#define MAGIC_TIC_RATE_DEMO     30          // Demo playback tic rate
#define MAGIC_NEWTIC_RATIO      1           // NEWTICRATIO default

#endif // P_MAGIC_H
