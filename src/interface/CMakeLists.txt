set(SRC_FILES
sdl/endtxt.cpp
sdl/i_main.cpp
sdl/i_net.cpp
sdl/i_sound.cpp
sdl/i_system.cpp
sdl/i_video.cpp
)

add_library(interface STATIC ${SRC_FILES})

target_include_directories(interface PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Check for SDL3_mixer
pkg_check_modules(SDL3MIXER QUIET sdl3-mixer)
if(SDL3MIXER_FOUND)
    target_include_directories(interface PRIVATE ${SDL3MIXER_INCLUDE_DIRS})
    target_link_libraries(interface PRIVATE ${SDL3MIXER_LIBRARIES})
else()
    target_compile_definitions(interface PRIVATE NO_MIXER)
endif()

# Check for OpenAL
find_package(OpenAL QUIET)
if(OpenAL_FOUND)
    target_compile_definitions(interface PRIVATE HAVE_OPENAL)
    target_include_directories(interface PRIVATE ${OpenAL_INCLUDE_DIR})
    target_link_libraries(interface PRIVATE ${OpenAL_LIBRARY})
    message(STATUS "OpenAL found - hardware-accelerated 3D audio enabled")
endif()

# Check for FluidSynth (MIDI backend)
find_package(FluidSynth QUIET)
if(FluidSynth_FOUND)
    target_compile_definitions(interface PRIVATE HAVE_FLUIDSYNTH)
    target_include_directories(interface PRIVATE ${FluidSynth_INCLUDE_DIRS})
    target_link_libraries(interface PRIVATE FluidSynth::libfluidsynth)
    message(STATUS "FluidSynth found - MIDI playback enabled")
endif()
