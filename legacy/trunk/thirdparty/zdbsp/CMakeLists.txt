# Standalone ZDBSP library build - isolated from parent compile definitions
cmake_minimum_required(VERSION 3.10)

project(ZDBSP_LIBRARY CXX)

# Set C++11 WITHOUT inheriting parent flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ZDBSP uses SSE but only matters on 32-bit; force it off for 64-bit
set(SSE_MATTERS NO)
set(BACKPATCH 0)
set(DISABLE_SSE 1)

# ZDBSP source files needed for library (NO main.cpp - that's for CLI tool)
set(ZDBSP_SOURCES
    nodebuild.cpp
    nodebuild_classify_nosse2.cpp
    nodebuild_events.cpp
    nodebuild_extract.cpp
    nodebuild_gl.cpp
    nodebuild_utility.cpp
    processor.cpp
    processor_udmf.cpp
    sc_man.cpp
    wad.cpp
    blockmapbuilder.cpp
    view.cpp
    zdbsp_library_stubs.cpp
)

set(ZDBSP_HEADERS
    doomdata.h
    nodebuild.h
    processor.h
    sc_man.h
    tarray.h
    templates.h
    wad.h
    workdata.h
    zdbsp.h
    xs_Float.h
    blockmapbuilder.h
)

# Build as static library
add_library(zdbsp STATIC ${ZDBSP_SOURCES} ${ZDBSP_HEADERS})

# Only ZDBSP's own include path - no parent paths
target_include_directories(zdbsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ZDBSP defines
target_compile_definitions(zdbsp PRIVATE -DDISABLE_SSE)

# Also build the CLI tool (zdbsp.exe) using same compiler settings
# Note: getopt.c is excluded - use system getopt instead (conflict with modern MinGW)
set(ZDBSP_CLI_SOURCES
    main.cpp
    nodebuild.cpp
    nodebuild_classify_nosse2.cpp
    nodebuild_events.cpp
    nodebuild_extract.cpp
    nodebuild_gl.cpp
    nodebuild_utility.cpp
    processor.cpp
    processor_udmf.cpp
    sc_man.cpp
    wad.cpp
    blockmapbuilder.cpp
    view.cpp
)

add_executable(zdbsp_cli EXCLUDE_FROM_ALL ${ZDBSP_CLI_SOURCES} ${ZDBSP_HEADERS})
target_include_directories(zdbsp_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(zdbsp_cli PRIVATE -DDISABLE_SSE)
# Use POSIX getopt from system
target_compile_definitions(zdbsp_cli PRIVATE _POSIX_C_SOURCE=200809L)
# Link with zlib for compression
target_link_libraries(zdbsp_cli PRIVATE C:/msys64/mingw64/lib/libz.a)
