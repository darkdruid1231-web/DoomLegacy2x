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
