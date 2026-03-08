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
