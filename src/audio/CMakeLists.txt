# OpenAL Sound System
# Uses OpenAL for hardware-accelerated 3D audio and SDL_sound for decoding

set(SRC_FILES
qmus2mid.cpp
s_sndseq.cpp
s_sound.cpp
sounds.cpp
)

add_library(audio STATIC ${SRC_FILES})

target_include_directories(audio PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Find OpenAL (optional - will use software fallback if not found)
find_package(OpenAL QUIET)
if(OpenAL_FOUND)
    message(STATUS "OpenAL found - hardware-accelerated 3D audio enabled")
    target_link_libraries(audio PRIVATE OpenAL::OpenAL)
    target_compile_definitions(audio PRIVATE HAVE_OPENAL)
else()
    message(STATUS "OpenAL not found - using software sound mixing")
    # Provide stub functions for when OpenAL is not available
    target_compile_definitions(audio PRIVATE OPENAL_NOT_AVAILABLE)
endif()

# Find SDL_sound (optional - for audio decoding)
# SDL_sound provides: WAV, OGG, FLAC, MP3, MID, AIFF, etc.
find_package(SDL_sound QUIET)
if(SDL_sound_FOUND)
    message(STATUS "SDL_sound found - using for audio decoding")
    target_link_libraries(audio PRIVATE ${SDL_SOUND_LIBRARIES})
    target_include_directories(audio PRIVATE ${SDL_SOUND_INCLUDE_DIRS})
    target_compile_definitions(audio PRIVATE HAVE_SDL_SOUND)
else()
    message(STATUS "SDL_sound not found - using legacy sound loading")
endif()

# Find Vorbis (for OGG support)
find_package(Vorbis QUIET)
if(Vorbis_FOUND)
    target_link_libraries(audio PRIVATE ${VORBIS_LIBRARIES})
    target_include_directories(audio PRIVATE ${VORBIS_INCLUDE_DIRS})
endif()

# Find FLAC (optional)
find_package(FLAC QUIET)
if(FLAC_FOUND)
    target_link_libraries(audio PRIVATE ${FLAC_LIBRARIES})
    target_include_directories(audio PRIVATE ${FLAC_INCLUDE_DIRS})
endif()
