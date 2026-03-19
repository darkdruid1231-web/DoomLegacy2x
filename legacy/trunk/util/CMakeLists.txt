set(SRC_FILES
command.cpp
console.cpp
dehacked.cpp
mnemonics.cpp
m_argv.cpp
m_archive.cpp
m_bbox.cpp
m_dll.cpp
m_fixed.cpp
m_misc.cpp
m_random.cpp
m_swap.cpp
md5.cpp
parser.cpp
tables.cpp
vfile.cpp
wad.cpp
w_wad.cpp
z_ascache.cpp
z_cache.cpp
z_zone.cpp
)

add_library(util STATIC ${SRC_FILES})

target_include_directories(util PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
