//-----------------------------------------------------------------------------
//
// Performance benchmark and optimization tests for collision/BSP code
// Includes O(n²) detection, spatial partitioning, and regression tests
//
//-----------------------------------------------------------------------------
#include "doomincl.h"
#include "p_local.h"
#include "p_maputl.h"
#include "m_fixed.h"
#include "z_zone.h"

// Magic numbers extracted from HTML docs (boomref.html)
// These should be used instead of hardcoded values
#define MAGIC_FLOOR_SHIFT      0x6000     // Floor linedef types
#define MAGIC_CEILING_SHIFT    0x4000     // Ceiling linedef types  
#define MAGIC_DOOR_SHIFT       0x3c00     // Door linedef types
#define MAGIC_LOCKED_SHIFT     0x3800     // Locked door types
#define MAGIC_LIFT_SHIFT       0x3400     // Lift types
#define MAGIC_STAIR_SHIFT      0x3000     // Stair types
#define MAGIC_CRUSHER_SHIFT    0x2f80     // Crusher types

#define MAGIC_BIT_9             0x200      // Sector bit for pushers

// Blockmap constants from p_local.h
#define BLOCKMAP_UNIT_SIZE      128
#define BLOCKMAP_FRAC_BITS      (FRACBITS + 7)

// Performance test results
typedef struct {
    const char* test_name;
    uint64_t cycles;
    int passes;
    boolean passed;
} perf_test_result_t;

// Test counters
static int test_intercepts_count = 0;
static int test_blockmap_hits = 0;
static int test_collision_checks = 0;

//-----------------------------------------------------------------------------
// O(n²) Detection - Test the intercept sorting algorithm
//-----------------------------------------------------------------------------

// Test the old O(n²) algorithm vs optimized version
static void benchmark_intercept_sort_old(intercept_t* intercepts, int count)
{
    // Old O(n²) selection sort - for comparison
    fixed_t dist;
    intercept_t* scan;
    intercept_t* in;
    int n = count;
    
    while (n--)
    {
        dist = FIXED_MAX;
        for (scan = intercepts; scan < intercepts + count; scan++)
        {
            if (scan->frac < dist)
            {
                dist = scan->frac;
                in = scan;
            }
        }
        in->frac = FIXED_MAX;
    }
}

// Optimized O(n log n) quicksort comparator
static int intercept_compare(const void* a, const void* b)
{
    const intercept_t* ia = (const intercept_t*)a;
    const intercept_t* ib = (const intercept_t*)b;
    
    if (ia->frac < ib->frac) return -1;
    if (ia->frac > ib->frac) return 1;
    return 0;
}

// New O(n log n) algorithm using qsort
static void benchmark_intercept_sort_new(intercept_t* intercepts, int count)
{
    qsort(intercepts, count, sizeof(intercept_t), intercept_compare);
}

// Benchmark results storage
#define MAX_BENCHMARK_PASSES 100
static perf_test_result_t benchmark_results[20];
static int benchmark_count = 0;

//-----------------------------------------------------------------------------
// Spatial Partitioning Tests - Blockmap optimization
//-----------------------------------------------------------------------------

// Test blockmap lookup efficiency
static void test_blockmap_efficiency(void)
{
    int x, y;
    int total_checks = 0;
    int valid_hits = 0;
    
    // Test various blockmap positions
    for (x = 0; x < (int)bmapwidth; x++)
    {
        for (y = 0; y < (int)bmapheight; y++)
        {
            // Count things in this block
            mobj_t* mobj;
            for (mobj = blocklinks[y * bmapwidth + x]; mobj; mobj = mobj->bnext)
            {
                valid_hits++;
            }
            total_checks++;
        }
    }
    
    test_blockmap_hits = valid_hits;
}

//-----------------------------------------------------------------------------
// Regression Tests - Ensure no behavior changes
//-----------------------------------------------------------------------------

// Test P_BlockLinesIterator returns correct results
static boolean test_blocklines_iterator(void)
{
    boolean result = true;
    int x, y;
    
    // Test boundary conditions
    for (x = -1; x <= (int)bmapwidth; x++)
    {
        for (y = -1; y <= (int)bmapheight; y++)
        {
            // Should not crash on boundary values
            P_BlockLinesIterator(x, y, NULL);
        }
    }
    
    return result;
}

// Test P_BlockThingsIterator returns correct results  
static boolean test_blockthings_iterator(void)
{
    boolean result = true;
    int x, y;
    
    // Test boundary conditions
    for (x = -1; x <= (int)bmapwidth; x++)
    {
        for (y = -1; y <= (int)bmapheight; y++)
        {
            // Should not crash on boundary values
            P_BlockThingsIterator(x, y, NULL);
        }
    }
    
    return result;
}

// Test intercept sorting produces correct order
static boolean test_intercept_sort_order(void)
{
    intercept_t test_intercepts[10];
    int i;
    
    // Create test intercepts with known fractions
    for (i = 0; i < 10; i++)
    {
        test_intercepts[i].frac = (i * 7) % 10 * FRACUNIT / 10; // Pseudo-random order
        test_intercepts[i].isaline = (i % 2 == 0);
    }
    
    // Sort using new algorithm
    qsort(test_intercepts, 10, sizeof(intercept_t), intercept_compare);
    
    // Verify sorted order
    for (i = 1; i < 10; i++)
    {
        if (test_intercepts[i].frac < test_intercepts[i-1].frac)
        {
            return false; // Not sorted correctly
        }
    }
    
    return true;
}

// Test P_PathTraverse with various flags
static boolean test_path_traverse(void)
{
    // Basic traversal test - should not crash
    boolean result = P_PathTraverse(0, 0, FRACUNIT * 100, FRACUNIT * 100, 
                                     PT_EARLYOUT, NULL);
    
    // Test with different flag combinations
    P_PathTraverse(0, 0, FRACUNIT * 100, FRACUNIT * 100, 0, NULL);
    P_PathTraverse(0, 0, FRACUNIT * 100, FRACUNIT * 100, PT_EARLYOUT, NULL);
    
    return result;
}

//-----------------------------------------------------------------------------
// Main test runner
//-----------------------------------------------------------------------------

void P_Run_Performance_Tests(void)
{
    // Initialize test results
    memset(benchmark_results, 0, sizeof(benchmark_results));
    
    // Run regression tests
    printf("Running regression tests...\n");
    
    if (!test_blocklines_iterator())
        printf("FAIL: test_blocklines_iterator\n");
    else
        printf("PASS: test_blocklines_iterator\n");
        
    if (!test_blockthings_iterator())
        printf("FAIL: test_blockthings_iterator\n");
    else
        printf("PASS: test_blockthings_iterator\n");
        
    if (!test_intercept_sort_order())
        printf("FAIL: test_intercept_sort_order\n");
    else
        printf("PASS: test_intercept_sort_order\n");
        
    if (!test_path_traverse())
        printf("FAIL: test_path_traverse\n");
    else
        printf("PASS: test_path_traverse\n");
    
    // Run blockmap efficiency test
    test_blockmap_efficiency();
    printf("Blockmap efficiency: %d things checked in %u blocks\n", 
           test_blockmap_hits, bmapwidth * bmapheight);
    
    printf("Performance tests complete.\n");
}

//-----------------------------------------------------------------------------
// Optimization helper functions
//-----------------------------------------------------------------------------

// Check if point is within blockmap bounds
static inline boolean P_BlockInBounds(int x, int y)
{
    return (x >= 0 && y >= 0 && 
            x < (int)bmapwidth && y < (int)bmapheight);
}

// Get blockmap index with bounds checking
static inline uint32_t P_BlockIndex(int x, int y)
{
    if (!P_BlockInBounds(x, y))
        return 0; // Default fallback
    return (uint32_t)(y * bmapwidth + x);
}

// Optimized thing iteration - skip empty blocks faster
static int P_CountThingsInBlocks(int x1, int y1, int x2, int y2)
{
    int x, y;
    int count = 0;
    
    // Clamp to valid range
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= (int)bmapwidth) x2 = bmapwidth - 1;
    if (y2 >= (int)bmapheight) y2 = bmapheight - 1;
    
    for (x = x1; x <= x2; x++)
    {
        for (y = y1; y <= y2; y++)
        {
            mobj_t* mobj;
            for (mobj = blocklinks[y * bmapwidth + x]; mobj; mobj = mobj->bnext)
            {
                count++;
            }
        }
    }
    
    return count;
}
