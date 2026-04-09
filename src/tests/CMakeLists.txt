# tests/CMakeLists.txt
# Test suite for DoomLegacy GL rendering and core systems.
#
# Tests are standalone executables with no game library linkage unless
# explicitly noted.  Run with: ctest --output-on-failure
#
# Google Test/Mock infrastructure:
# - GTest/GMock are fetched via FetchContent in tests/libraries/CMakeLists.txt
# - Mock objects (MockWad, MockSound) live in tests/libraries/
# - Interface headers live in include/interfaces/

# ---------------------------------------------------------------------------
# Test support libraries (GTest/GMock + mock objects)
# ---------------------------------------------------------------------------
add_subdirectory(libraries)

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
# Links against util_base (no interface deps needed).
# ---------------------------------------------------------------------------
add_executable(test_fixed_t unit/test_fixed_t.cpp)
target_include_directories(test_fixed_t PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_fixed_t util_base)
target_compile_definitions(test_fixed_t PRIVATE SDL_MAIN_HANDLED)
add_test(NAME fixed_t COMMAND test_fixed_t)

# ---------------------------------------------------------------------------
# test_fixed_t_gtest — Google Test version of fixed-point arithmetic tests.
# Demonstrates parameterized testing with GTest.
# ---------------------------------------------------------------------------
add_executable(test_fixed_t_gtest unit/test_fixed_t_gtest.cpp)
target_include_directories(test_fixed_t_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(test_fixed_t_gtest PRIVATE
    util_base
    gtest
    TestInterfaces
)
target_compile_definitions(test_fixed_t_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME fixed_t_gtest COMMAND test_fixed_t_gtest)

# ---------------------------------------------------------------------------
# test_vector — vec_t<fixed_t> operations.
# Links against util_base (no interface deps needed).
# ---------------------------------------------------------------------------
add_executable(test_vector unit/test_vector.cpp)
target_include_directories(test_vector PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(test_vector util_base)
target_compile_definitions(test_vector PRIVATE SDL_MAIN_HANDLED)
add_test(NAME vector COMMAND test_vector)

# ---------------------------------------------------------------------------
# test_console — console variable (CV_*) flag constants.
#
# test_console_stubs.cpp provides CONS_Printf (preventing console.cpp.o from
# being pulled out of libutil.a) plus stubs for the remaining symbols that
# command.cpp.o needs directly (game, LocalPlayers, devparm, I_Error,
# LNetInterface::SendNetVar).  The stubs file must be compiled into the
# executable (not a separate library) so its .o precedes libutil.a on the
# linker command line.
# ---------------------------------------------------------------------------
add_executable(test_console
    unit/test_console.cpp
    unit/test_console_stubs.cpp
)
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

# ---------------------------------------------------------------------------
# test_wad_lump_gtest — Google Test version of WAD lump lookup tests.
# Demonstrates GTest/Mock infrastructure with MockWadRepository.
# ---------------------------------------------------------------------------
add_executable(test_wad_lump_gtest unit/test_wad_lump_gtest.cpp)
target_include_directories(test_wad_lump_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
)
target_link_libraries(test_wad_lump_gtest PRIVATE
    MockWad
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_wad_lump_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME wad_lump_gtest COMMAND test_wad_lump_gtest)

# ---------------------------------------------------------------------------
# test_wad_lump_reader — WadLumpReader component tests with MockWadRepository.
# Demonstrates full DI pattern: WadLumpReader -> IWadRepository -> MockWadRepository.
# ---------------------------------------------------------------------------
add_executable(test_wad_lump_reader unit/test_wad_lump_reader.cpp)
target_include_directories(test_wad_lump_reader PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/util
)
target_link_libraries(test_wad_lump_reader PRIVATE
    MockWad
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_wad_lump_reader PRIVATE SDL_MAIN_HANDLED)
add_test(NAME wad_lump_reader COMMAND test_wad_lump_reader)

# ---------------------------------------------------------------------------
# test_game_state_gtest — IGameStateProvider DI pattern demonstration.
# ---------------------------------------------------------------------------
add_executable(test_game_state_gtest unit/test_game_state_gtest.cpp)
target_include_directories(test_game_state_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(test_game_state_gtest PRIVATE
    MockGame
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_game_state_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME game_state_gtest COMMAND test_game_state_gtest)

# ---------------------------------------------------------------------------
# test_cvar_gtest — ICVarProvider DI pattern demonstration.
# ---------------------------------------------------------------------------
add_executable(test_cvar_gtest unit/test_cvar_gtest.cpp)
target_include_directories(test_cvar_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
)
target_link_libraries(test_cvar_gtest PRIVATE
    MockCVar
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_cvar_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME cvar_gtest COMMAND test_cvar_gtest)

# ---------------------------------------------------------------------------
# test_render_backend_gtest — IRenderBackend DI pattern demonstration.
# ---------------------------------------------------------------------------
add_executable(test_render_backend_gtest unit/test_render_backend_gtest.cpp)
target_include_directories(test_render_backend_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(test_render_backend_gtest PRIVATE
    MockRender
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_render_backend_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME render_backend_gtest COMMAND test_render_backend_gtest)

# ---------------------------------------------------------------------------
# test_sound_emitter_gtest — SoundEmitterComponent with MockSoundSystem.
# ---------------------------------------------------------------------------
add_executable(test_sound_emitter_gtest unit/test_sound_emitter_gtest.cpp)
target_include_directories(test_sound_emitter_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(test_sound_emitter_gtest PRIVATE
    MockSound
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_sound_emitter_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME sound_emitter_gtest COMMAND test_sound_emitter_gtest)

# ---------------------------------------------------------------------------
# test_hud_gtest — HUDComponent with MockGameStateProvider.
# ---------------------------------------------------------------------------
add_executable(test_hud_gtest unit/test_hud_gtest.cpp)
target_include_directories(test_hud_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(test_hud_gtest PRIVATE
    MockGame
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_hud_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME hud_gtest COMMAND test_hud_gtest)

# ---------------------------------------------------------------------------
# test_bsp_traverser_gtest — BSPTraverserComponent with MockRenderBackend.
# ---------------------------------------------------------------------------
add_executable(test_bsp_traverser_gtest unit/test_bsp_traverser_gtest.cpp)
target_include_directories(test_bsp_traverser_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(test_bsp_traverser_gtest PRIVATE
    MockRender
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_bsp_traverser_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME bsp_traverser_gtest COMMAND test_bsp_traverser_gtest)

# ---------------------------------------------------------------------------
# test_bsp_compiler_gtest — BspCompiler (ZDBSP library) tests.
# Tests that BspCompiler produces valid GL nodes from a minimal map.
# ---------------------------------------------------------------------------
add_executable(test_bsp_compiler_gtest unit/test_bsp_compiler_gtest.cpp)
target_include_directories(test_bsp_compiler_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/thirdparty/zdbsp
    ${CMAKE_SOURCE_DIR}/thirdparty/zdbsp/zlib
)
# Link order: Use linker group to handle circular dependencies between zdbsp and zlib
# zdbsp uses zlib functions, so zlib must be resolvable after zdbsp is processed
target_link_libraries(test_bsp_compiler_gtest PRIVATE
    "-Wl,--start-group"
    doomengine
    gtest
    zdbsp
    ZLIB::ZLIB
    "-Wl,--end-group"
)
target_compile_definitions(test_bsp_compiler_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME bsp_compiler_gtest COMMAND test_bsp_compiler_gtest)

# ---------------------------------------------------------------------------
# test_graphics_settings_gtest — GraphicsSettingsComponent with MockCVarProvider.
# ---------------------------------------------------------------------------
add_executable(test_graphics_settings_gtest unit/test_graphics_settings_gtest.cpp)
target_include_directories(test_graphics_settings_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
)
target_link_libraries(test_graphics_settings_gtest PRIVATE
    MockCVar
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_graphics_settings_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME graphics_settings_gtest COMMAND test_graphics_settings_gtest)

# ---------------------------------------------------------------------------
# test_network_provider_gtest — INetworkProvider DI pattern demonstration.
# ---------------------------------------------------------------------------
add_executable(test_network_provider_gtest unit/test_network_provider_gtest.cpp)
target_include_directories(test_network_provider_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(test_network_provider_gtest PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_network_provider_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME network_provider_gtest COMMAND test_network_provider_gtest)

# ---------------------------------------------------------------------------
# test_input_provider_gtest — IInputProvider DI pattern demonstration.
# ---------------------------------------------------------------------------
add_executable(test_input_provider_gtest unit/test_input_provider_gtest.cpp)
target_include_directories(test_input_provider_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
    ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(test_input_provider_gtest PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_input_provider_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME input_provider_gtest COMMAND test_input_provider_gtest)

# ---------------------------------------------------------------------------
# test_renderer_gtest — IRenderer DI pattern demonstration.
# ---------------------------------------------------------------------------
add_executable(test_renderer_gtest unit/test_renderer_gtest.cpp)
target_include_directories(test_renderer_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(test_renderer_gtest PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(test_renderer_gtest PRIVATE SDL_MAIN_HANDLED)
add_test(NAME renderer_gtest COMMAND test_renderer_gtest)
