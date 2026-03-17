set(SRC_FILES
sdl/endtxt.cpp
sdl/i_cdmus.cpp
sdl/i_main.cpp
sdl/i_net.cpp
sdl/i_sound.cpp
sdl/i_system.cpp
sdl/i_video.cpp
)

add_library(interface OBJECT ${SRC_FILES})

target_include_directories(interface PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Check for OpenAL
find_package(OpenAL QUIET)
if(OpenAL_FOUND)
    target_compile_definitions(interface PRIVATE HAVE_OPENAL)
    target_include_directories(interface PRIVATE ${OpenAL_INCLUDE_DIR})
    target_link_libraries(interface PRIVATE ${OpenAL_LIBRARY})
    message(STATUS "OpenAL found - hardware-accelerated 3D audio enabled")
endif()
