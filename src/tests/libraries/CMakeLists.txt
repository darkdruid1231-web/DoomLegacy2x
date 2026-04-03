# tests/libraries/CMakeLists.txt
# Test support library containing mock objects and test utilities.
# These mocks implement the interface ABCs defined in include/interfaces/
# and are linked by test executables to enable unit testing without
# full system dependencies.

cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# GoogleTest / GoogleMock via FetchContent
# ---------------------------------------------------------------------------
include(FetchContent)

set(GOOGLETEST_VERSION 1.12.1)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(install_gtest OFF CACHE BOOL "" FORCE)
set(gtest_build_tests OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-${GOOGLETEST_VERSION}
)

FetchContent_MakeAvailable(googletest)

# Create aliases for consistent naming (GTest::GTest, GTest::Mock)
# GoogleTest's CMake creates 'gtest' and 'gmock' targets
add_library(GTest::GTest ALIAS gtest)
add_library(GTest::Mock ALIAS gmock)

# ---------------------------------------------------------------------------
# Interface headers (these don't build, they just set include paths)
# ---------------------------------------------------------------------------
add_library(TestInterfaces INTERFACE)
target_include_directories(TestInterfaces INTERFACE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
)
target_link_libraries(TestInterfaces INTERFACE gtest)

# ---------------------------------------------------------------------------
# Wad mock implementation
# ---------------------------------------------------------------------------
add_library(MockWad STATIC
    mock_wad.cpp
)
target_include_directories(MockWad PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
)
target_link_libraries(MockWad PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockWad PRIVATE SDL_MAIN_HANDLED)

# ---------------------------------------------------------------------------
# Sound mock implementation
# ---------------------------------------------------------------------------
add_library(MockSound STATIC
    mock_sound.cpp
)
target_include_directories(MockSound PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${SDL3_INCLUDE_DIRS}
)
target_link_libraries(MockSound PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockSound PRIVATE SDL_MAIN_HANDLED)

# ---------------------------------------------------------------------------
# Game mock implementation
# ---------------------------------------------------------------------------
add_library(MockGame STATIC
    mock_game.cpp
)
target_include_directories(MockGame PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
    ${SDL3_INCLUDE_DIRS}
)
target_link_libraries(MockGame PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockGame PRIVATE SDL_MAIN_HANDLED)

# ---------------------------------------------------------------------------
# CVar mock implementation
# ---------------------------------------------------------------------------
add_library(MockCVar STATIC
    mock_cvar.cpp
)
target_include_directories(MockCVar PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
)
target_link_libraries(MockCVar PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockCVar PRIVATE SDL_MAIN_HANDLED)

# ---------------------------------------------------------------------------
# Render mock implementation
# ---------------------------------------------------------------------------
add_library(MockRender STATIC
    mock_render.cpp
)
target_include_directories(MockRender PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(MockRender PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockRender PRIVATE SDL_MAIN_HANDLED)

# ---------------------------------------------------------------------------
# Game Backend mock implementation
# ---------------------------------------------------------------------------
add_library(MockGameBackend STATIC
    mock_game_backend.cpp
)
target_include_directories(MockGameBackend PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(MockGameBackend PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockGameBackend PRIVATE SDL_MAIN_HANDLED)

# ---------------------------------------------------------------------------
# Sound Player mock implementation
# ---------------------------------------------------------------------------
add_library(MockSoundPlayer STATIC
    mock_sound_player.cpp
)
target_include_directories(MockSoundPlayer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${SDL3_INCLUDE_DIRS}
)
target_link_libraries(MockSoundPlayer PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockSoundPlayer PRIVATE SDL_MAIN_HANDLED)

# ---------------------------------------------------------------------------
# Network Provider mock implementation
# ---------------------------------------------------------------------------
add_library(MockNetworkProvider STATIC
    mock_network_provider.cpp
)
target_include_directories(MockNetworkProvider PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(MockNetworkProvider PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockNetworkProvider PRIVATE SDL_MAIN_HANDLED)

# ---------------------------------------------------------------------------
# Input Provider mock implementation
# ---------------------------------------------------------------------------
add_library(MockInputProvider STATIC
    mock_input_provider.cpp
)
target_include_directories(MockInputProvider PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
    ${SDL3_INCLUDE_DIRS}
)
target_link_libraries(MockInputProvider PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockInputProvider PRIVATE SDL_MAIN_HANDLED)

# ---------------------------------------------------------------------------
# Render mock implementation
# ---------------------------------------------------------------------------
add_library(MockRenderer STATIC
    mock_renderer.cpp
)
target_include_directories(MockRenderer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/game
)
target_link_libraries(MockRenderer PRIVATE
    gtest
    gmock
    TestInterfaces
)
target_compile_definitions(MockRenderer PRIVATE SDL_MAIN_HANDLED)

