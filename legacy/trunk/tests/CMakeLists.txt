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
# test_actor — actor flag constant values.
# Links against util for fixed_t and related types.
# ---------------------------------------------------------------------------
add_executable(test_actor unit/test_actor.cpp)
# SDL2 include dirs are resolved by the parent CMakeLists before add_subdirectory(tests).
target_include_directories(test_actor PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_actor util)
# SDL_main.h redefines main→SDL_main unless we declare we handle main ourselves.
# Without this, MinGW falls through to the GUI CRT startup looking for WinMain.
target_compile_definitions(test_actor PRIVATE SDL_MAIN_HANDLED)
add_test(NAME actor COMMAND test_actor)
