cmake_minimum_required(VERSION 3.16)

project(doomlegacy C CXX)

# Disable tests for now to avoid Catch2 issues
option(BUILD_TESTING "Build tests" OFF)

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
add_compile_options(-fpermissive)

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

# GLEW - use shared library version for MinGW compatibility
set(USE_GLEW ON)
find_path(GLEW_INCLUDE_DIR GL/glew.h)
# Use shared GLEW library (dll.a) instead of static - works better with MinGW
set(GLEW_LIBRARY "C:/msys64/mingw64/lib/libglew32.dll.a" CACHE FILEPATH "GLEW library")
set(GLEW_DLL "C:/msys64/mingw64/bin/libglew32.dll" CACHE FILEPATH "GLEW DLL")
if(NOT GLEW_INCLUDE_DIR OR NOT GLEW_LIBRARY)
    message(FATAL_ERROR "GLEW not found!")
endif()
include_directories(${GLEW_INCLUDE_DIR})
link_directories(${GLEW_LIBRARY_DIR})

# ZLIB
find_package(ZLIB REQUIRED)
# PNG
find_package(PNG REQUIRED)
# JPEG
find_package(JPEG REQUIRED)
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
# Flex/Bison needed for grammars
# Set paths directly - skip CMake's find_package as it has issues with MSYS2 tools
set(FLEX_EXECUTABLE "C:/msys64/mingw64/bin/flex.exe" CACHE FILEPATH "Flex executable")
set(BISON_EXECUTABLE "C:/msys64/usr/bin/bison.exe" CACHE FILEPATH "Bison executable")
set(LEMON_EXECUTABLE "C:/msys64/mingw64/bin/lemon.exe" CACHE FILEPATH "Lemon executable")

add_subdirectory(audio)
add_subdirectory(video)
add_subdirectory(util)
add_subdirectory(net)
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
    util
    doomengine
    video
    grammars
    net
    ${GLEW_LIBRARY}
    ${OPENGL_LIBRARIES}
    "-Wl,--end-group"
    ${SDL2_LIBRARIES}
    ${SDL2MIXER_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARY}
    ${JPEG_LIBRARY}
    ${ENET_LIBRARIES}
)

# Workaround for multiple definition errors with dummy Shader classes
# The Shader/ShaderProg classes are compiled in both video and grammars with NO_SHADERS
target_link_options(doomlegacy PRIVATE "-Wl,--allow-multiple-definition")
