cmake_minimum_required(VERSION 3.16)

project(doomlegacy C CXX)

option(BUILD_TESTING "Build tests" ON)
option(ENABLE_IWYU "Run Include-what-you-use on new interface headers" OFF)

# IWYU (Include What You Use) — only available if explicitly enabled
# and only runs on interface headers in CI, not blocking for local dev
find_program(IWYU_COMMAND NAMES include-what-you-use iwyu)
if(IWYU_COMMAND AND ENABLE_IWYU)
  message(STATUS "IWYU found: ${IWYU_COMMAND}")
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_COMMAND};--transitive")
else()
  message(STATUS "IWYU not enabled or not found (found: ${IWYU_COMMAND})")
endif()

# Global include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include_stubs
    ${CMAKE_SOURCE_DIR}/tnl
)

# Global compile options
# Workaround for GCC 13+ / MinGW intrinsics conflicts
add_compile_definitions(_XSAVEINTRIN_H_INCLUDED _CPUID_H_INCLUDED _XGETBV_DEFINED WIN32_LEAN_AND_MEAN NOMINMAX)
# SDL2 only
add_compile_definitions(SDL SDL2)
# Use TNL stub (no actual TNL library)
add_compile_definitions(TNL_STUB_BUILD)
# Math constants (M_PI not in C++11 by default)
add_compile_definitions(_USE_MATH_DEFINES)

# Set C++ standard
# Use C++11 to avoid std::byte (introduced in C++17)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Suppress warnings for legacy code
add_compile_options(-fpermissive -w)

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
if(WIN32)
  set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})
endif()

if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
   message(FATAL_ERROR "In-tree build attempt detected, aborting. Set your build dir outside your source dir, delete CMakeCache.txt from source root and try again.")
endif()

set(DL_VERSION_MAJOR "1")
set(DL_VERSION_MINOR "9")
set(DL_VERSION_PATCH "9")
set(DL_VERSION "${DL_VERSION_MAJOR}.${DL_VERSION_MINOR}.${DL_VERSION_PATCH}")

# External libraries. First SDL.
option(USE_SDL2 "Enable SDL2 support" ON)

if(USE_SDL2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    if(SDL2_LIBRARY_DIRS)
        link_directories(${SDL2_LIBRARY_DIRS})
    endif()
    option(USE_SOUND "Enable sound support" ON)
    if(USE_SOUND STREQUAL "ON")
        pkg_check_modules(SDL2MIXER SDL2_mixer)
        if(SDL2MIXER_LIBRARY_DIRS)
            link_directories(${SDL2MIXER_LIBRARY_DIRS})
        endif()
        if(SDL2MIXER_FOUND)
            set(HAVE_MIXER 1)
            list(APPEND SOURCES interface/sdl/i_sound.cpp)
        else()
            message(STATUS "SDL2_mixer not found, disabling sound")
        endif()
    endif()
    # FreeType for TrueType font support
    option(USE_TTF "Enable TrueType font support" ON)
    if(USE_TTF STREQUAL "ON")
        pkg_check_modules(FREETYPE freetype2)
        if(FREETYPE_FOUND)
            set(HAVE_TTF 1)
            add_compile_definitions(HAVE_TTF)
            include_directories(${FREETYPE_INCLUDE_DIRS})
            if(FREETYPE_LIBRARY_DIRS)
                link_directories(${FREETYPE_LIBRARY_DIRS})
            endif()
            message(STATUS "FreeType found, TrueType fonts enabled")
        else()
            message(STATUS "FreeType not found, TrueType fonts disabled")
        endif()
    endif()
    if(SDL2_FOUND)
        include_directories(${SDL2_INCLUDE_DIRS})
    endif()
else()
    message(STATUS "SDL2 disabled, using SDL1")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL REQUIRED sdl)
    include_directories(${SDL_INCLUDE_DIRS})
    add_compile_definitions(SDL)
    set(SDL2_LIBRARIES ${SDL_LIBRARIES})
    set(SDL2_INCLUDE_DIRS ${SDL_INCLUDE_DIRS})
endif()

# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# GLEW
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

# ZLIB
find_package(ZLIB REQUIRED)
# PNG
find_package(PNG REQUIRED)
# JPEG
find_package(JPEG REQUIRED)

# OpenAL for hardware-accelerated 3D audio
find_package(OpenAL QUIET)
if(OpenAL_FOUND)
    add_compile_definitions(HAVE_OPENAL)
    message(STATUS "OpenAL found - hardware-accelerated 3D audio enabled")
else()
    message(STATUS "OpenAL not found - using software audio")
endif()

# ENet
pkg_check_modules(ENET libenet)
if(ENET_LIBRARY_DIRS)
    link_directories(${ENET_LIBRARY_DIRS})
endif()
if(ENET_FOUND)
    include_directories(${ENET_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "ENet not found!")
endif()
# Flex/Bison/Lemon needed for grammars
find_program(FLEX_EXECUTABLE NAMES flex flex.exe REQUIRED)
find_program(BISON_EXECUTABLE NAMES bison bison.exe REQUIRED)
find_program(LEMON_EXECUTABLE NAMES lemon lemon.exe REQUIRED)
message(STATUS "Flex:  ${FLEX_EXECUTABLE}")
message(STATUS "Bison: ${BISON_EXECUTABLE}")
message(STATUS "Lemon: ${LEMON_EXECUTABLE}")

add_subdirectory(audio)
add_subdirectory(video)
add_subdirectory(util)
add_subdirectory(util_base)
add_subdirectory(net)
add_subdirectory(thirdparty/zdbsp)
add_subdirectory(engine)
add_subdirectory(grammars)
add_subdirectory(interface)

# Main executable - interface provides main()
# Get interface sources for the executable
set(INTERFACE_SOURCES
    interface/sdl/endtxt.cpp
    interface/sdl/i_cdmus.cpp
    interface/sdl/i_main.cpp
    interface/sdl/i_net.cpp
    interface/sdl/i_sound.cpp
    interface/sdl/i_system.cpp
    interface/sdl/i_video.cpp
)

add_executable(doomlegacy WIN32 ${INTERFACE_SOURCES})
# Important: link order matters!
# - video must come before grammars (grammars uses Shader/ShaderProg from video)
# - Use linker groups (-Wl,--start-group ... -Wl,--end-group) to handle circular dependencies
#   between video library and OpenGL/GLEW
# Link order with groups to handle circular dependencies between libraries
# video and grammars need each other (video defines Shader classes used by grammars)
# Link order: main libraries first, then a group for interdependent libraries
# doomengine must come before video (video uses engine symbols)
# video must come before grammars (grammars uses Shader from video)
# Use linker group to handle circular dependencies between all libraries
# util must be in the group (video/grammars need cacheitem_t from util)
target_link_libraries(doomlegacy PRIVATE
    audio
    "-Wl,--start-group"
    util_base
    util
    doomengine
    video
    grammars
    net
    ${GLEW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    "-Wl,--end-group"
    ${SDL2_LIBRARIES}
    ${SDL2MIXER_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARY}
    ${JPEG_LIBRARY}
    ${ENET_LIBRARIES}
)

# Workaround for multiple definition errors with dummy Shader classes
# The Shader/ShaderProg classes are compiled in both video and grammars with NO_SHADERS
target_link_options(doomlegacy PRIVATE "-Wl,--allow-multiple-definition")

# ---------------------------------------------------------------------------
# test_menu_navigation — WidgetPanel navigation logic (HandleNavKey).
# Compiled with interface sources so I_Error and other interface symbols are
# available. Links against doomlegacy to get all engine symbols.
# ---------------------------------------------------------------------------
add_executable(test_menu_navigation tests/unit/test_menu_navigation.cpp ${INTERFACE_SOURCES})
target_include_directories(test_menu_navigation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/engine
)
target_link_libraries(test_menu_navigation PRIVATE
    audio
    "-Wl,--start-group"
    util
    doomengine
    video
    grammars
    net
    ${GLEW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    "-Wl,--end-group"
    ${SDL2_LIBRARIES}
    ${SDL2MIXER_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARY}
    ${JPEG_LIBRARY}
    ${ENET_LIBRARIES}
)
target_link_options(test_menu_navigation PRIVATE "-Wl,--allow-multiple-definition")
target_compile_definitions(test_menu_navigation PRIVATE SDL_MAIN_HANDLED)
add_test(NAME menu_navigation COMMAND test_menu_navigation)

# ---------------------------------------------------------------------------
# test_menu_widget — Widget draw call verification via FakeRenderBackend.
# ---------------------------------------------------------------------------
add_executable(test_menu_widget tests/unit/test_menu_widget.cpp ${INTERFACE_SOURCES})
target_include_directories(test_menu_widget PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/engine
)
target_link_libraries(test_menu_widget PRIVATE
    audio
    "-Wl,--start-group"
    util
    doomengine
    video
    grammars
    net
    ${GLEW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    "-Wl,--end-group"
    ${SDL2_LIBRARIES}
    ${SDL2MIXER_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARY}
    ${JPEG_LIBRARY}
    ${ENET_LIBRARIES}
)
target_link_options(test_menu_widget PRIVATE "-Wl,--allow-multiple-definition")
target_compile_definitions(test_menu_widget PRIVATE SDL_MAIN_HANDLED)
add_test(NAME menu_widget COMMAND test_menu_widget)

# ---------------------------------------------------------------------------
# test_menu_draw — classic Menu::DrawMenu() draw-call sequence via FakeMenuBackend.
# Links against doomengine for Menu, IMenuBackend, FakeMenuBackend, menuitem_t.
# Uses i_error_stub.cpp to provide I_Error without full interface layer.
# ---------------------------------------------------------------------------
add_executable(test_menu_draw tests/unit/test_menu_draw.cpp tests/unit/i_error_stub.cpp ${INTERFACE_SOURCES})
target_include_directories(test_menu_draw PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/engine)
target_link_libraries(test_menu_draw PRIVATE
    audio
    "-Wl,--start-group"
    util
    doomengine
    video
    grammars
    net
    ${GLEW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    "-Wl,--end-group"
    ${SDL2_LIBRARIES}
    ${SDL2MIXER_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARY}
    ${JPEG_LIBRARY}
    ${ENET_LIBRARIES})
target_compile_definitions(test_menu_draw PRIVATE SDL_MAIN_HANDLED)
target_link_options(test_menu_draw PRIVATE "-Wl,--allow-multiple-definition")
add_test(NAME menu_draw COMMAND test_menu_draw)

# Tests — added after all include dirs and libraries are resolved.
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
