set(SRC_FILES
png.cpp
jpeg.cpp
screen.cpp
v_video.cpp
r_font.cpp
r_render.cpp
r_bsp.cpp
r_data.cpp
r_draw.cpp
r_draw8.cpp
r_draw16.cpp
r_main.cpp
r_plane.cpp
r_segs.cpp
r_sky.cpp
r_splats.cpp
r_sprite.cpp
r_things.cpp
r_anim.cpp
hardware/oglrenderer.cpp
hardware/md3.cpp
hardware/oglshaders.cpp
hardware/oglhelpers.cpp
hardware/hw_gbuffer.cpp
hardware/hw_postfx.cpp
hardware/hw_lightdefs.cpp
hardware/hw_quadbatch.cpp
hardware/hw_shadowmap.cpp
)

add_library(video STATIC ${SRC_FILES})

target_link_libraries(video PUBLIC ${GLEW_LIBRARY} ${OPENGL_LIBRARIES} stdc++)

target_include_directories(video PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/hardware
    ${GLEW_INCLUDE_DIR}
)