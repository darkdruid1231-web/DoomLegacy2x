# tests/CMakeLists.txt
# Test suite for DoomLegacy GL rendering and core systems.
#
# Tests are standalone executables with no game library linkage unless
# explicitly noted.  Run with: ctest --output-on-failure

# ---------------------------------------------------------------------------
# test_sprite_logic — pure-math replicas of sprite rendering formulas.
# Tests: rotation index, Z-clamp, frame-clamp (no game/GL deps).
# ---------------------------------------------------------------------------
add_executable(test_sprite_logic unit/test_sprite_logic.cpp)
target_include_directories(test_sprite_logic PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
)
add_test(NAME sprite_logic COMMAND test_sprite_logic)

# ---------------------------------------------------------------------------
# Shared helper macro: all tests that link against util need SDL_MAIN_HANDLED
# so SDL_main.h does not rename main()→SDL_main() and trigger the MinGW
# GUI-CRT WinMain fallback.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# test_actor — actor flag constant values.
# Links against util for fixed_t and related types.
# ---------------------------------------------------------------------------
add_executable(test_actor unit/test_actor.cpp)
target_include_directories(test_actor PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_actor util)
target_compile_definitions(test_actor PRIVATE SDL_MAIN_HANDLED)
add_test(NAME actor COMMAND test_actor)

# ---------------------------------------------------------------------------
# test_fixed_t — fixed_t arithmetic (add, sub, mul, div, precision, negative).
# ---------------------------------------------------------------------------
add_executable(test_fixed_t unit/test_fixed_t.cpp)
target_include_directories(test_fixed_t PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_fixed_t util)
target_compile_definitions(test_fixed_t PRIVATE SDL_MAIN_HANDLED)
add_test(NAME fixed_t COMMAND test_fixed_t)

# ---------------------------------------------------------------------------
# test_vector — vec_t<fixed_t> operations.
# ---------------------------------------------------------------------------
add_executable(test_vector unit/test_vector.cpp)
target_include_directories(test_vector PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_vector util)
target_compile_definitions(test_vector PRIVATE SDL_MAIN_HANDLED)
add_test(NAME vector COMMAND test_vector)

# ---------------------------------------------------------------------------
# test_console — console variable (CV_*) flag constants.
# ---------------------------------------------------------------------------
add_executable(test_console unit/test_console.cpp)
target_include_directories(test_console PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_console util)
target_compile_definitions(test_console PRIVATE SDL_MAIN_HANDLED)
add_test(NAME console COMMAND test_console)

# ---------------------------------------------------------------------------
# test_video — video subsystem constant and type checks.
# ---------------------------------------------------------------------------
add_executable(test_video unit/test_video.cpp)
target_include_directories(test_video PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_video util)
target_compile_definitions(test_video PRIVATE SDL_MAIN_HANDLED)
add_test(NAME video COMMAND test_video)

# ---------------------------------------------------------------------------
# test_bot — bot AI flag and constant values.
# ---------------------------------------------------------------------------
add_executable(test_bot unit/test_bot.cpp)
target_include_directories(test_bot PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_bot util)
target_compile_definitions(test_bot PRIVATE SDL_MAIN_HANDLED)
add_test(NAME bot COMMAND test_bot)

# ---------------------------------------------------------------------------
# test_tnl_packets — TNL packet field structure checks.
# ---------------------------------------------------------------------------
add_executable(test_tnl_packets unit/test_tnl_packets.cpp)
target_include_directories(test_tnl_packets PRIVATE ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/tnl)
target_link_libraries(test_tnl_packets util)
target_compile_definitions(test_tnl_packets PRIVATE SDL_MAIN_HANDLED)
add_test(NAME tnl_packets COMMAND test_tnl_packets)

# ---------------------------------------------------------------------------
# test_mapinfo — MapInfo struct field and enum checks.
# ---------------------------------------------------------------------------
add_executable(test_mapinfo unit/test_mapinfo.cpp)
target_include_directories(test_mapinfo PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_mapinfo util)
target_compile_definitions(test_mapinfo PRIVATE SDL_MAIN_HANDLED)
add_test(NAME mapinfo COMMAND test_mapinfo)

# ---------------------------------------------------------------------------
# test_acs — ACS script VM type and operation checks.
# ---------------------------------------------------------------------------
add_executable(test_acs unit/test_acs.cpp)
target_include_directories(test_acs PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_acs util)
target_compile_definitions(test_acs PRIVATE SDL_MAIN_HANDLED)
add_test(NAME acs COMMAND test_acs)
