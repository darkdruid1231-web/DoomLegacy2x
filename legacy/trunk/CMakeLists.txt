project(doomlegacy C CXX)
cmake_minimum_required(VERSION 2.8.0)

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

set(DL_VERSION "${CF_VERSION_MAJOR}.${CF_VERSION_MINOR}.${CF_VERSION_PATCH}")

# All CMake include stuffs.
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckIncludeFileCXX)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(c0flag "-std=c++0x -fpermissive -w")
    CHECK_CXX_COMPILER_FLAG(${c0flag} HAS_C0X)
    if(NOT ${HAS_C0X})
        message(FATAL_ERROR "Your compiler does not support c++0x features. Can not build.")
    endif()
    add_definitions(${c0flag})
else()
    message(FATAL_ERROR "Only GNU C++ is supported at the moment.")
endif()

# External libraries. First SDL.
find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)
include_directories(${SDL_INCLUDE_DIR} ${SDL_MIXER_DIR})
add_definitions("-DSDL")

# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# Then OpenTNL.
#set(HAS_TNL_HEADER 1)
#message("TNL: ${HAS_TNL_HEADER}")

# Look for TNL in multiple locations: system, local tnl/ directory, or hardcoded path
set(TNL_LIBRARY "TNL_LIBRARY-NOTFOUND")
set(TOMCRYPT_LIBRARY "TOMCRYPT_LIBRARY-NOTFOUND")

# Try local tnl directory first (Windows development)
if(EXISTS "${CMAKE_SOURCE_DIR}/tnl/libtnl.a")
    set(TNL_LIBRARY "${CMAKE_SOURCE_DIR}/tnl/libtnl.a")
    set(TOMCRYPT_LIBRARY "${CMAKE_SOURCE_DIR}/tnl/libtomcrypt.a")
endif()

# Try Linux path
if(NOT TNL_LIBRARY AND EXISTS "/home/geoffrey/tnl/lib/libtnl.a")
    set(TNL_LIBRARY "/home/geoffrey/tnl/lib/libtnl.a")
    set(TOMCRYPT_LIBRARY "/home/geoffrey/tnl/lib/libtomcrypt.a")
endif()

# Try system library (including MSYS2 MinGW)
if(NOT TNL_LIBRARY)
    find_library(TNL_LIBRARY tnl)
    find_library(TOMCRYPT_LIBRARY tomcrypt PATHS /c/msys64/mingw64/lib)
endif()

# If TNL library not found, build from source
if(TNL_LIBRARY STREQUAL "TNL_LIBRARY-NOTFOUND")
    message(STATUS "Building TNL from source...")

    # Collect TNL source files (minimal set for basic functionality)
    set(TNL_SOURCES
        ${CMAKE_SOURCE_DIR}/tnl/assert.cpp
        ${CMAKE_SOURCE_DIR}/tnl/bitStream.cpp
        ${CMAKE_SOURCE_DIR}/tnl/byteBuffer.cpp
        ${CMAKE_SOURCE_DIR}/tnl/connectionStringTable.cpp
        ${CMAKE_SOURCE_DIR}/tnl/dataChunker.cpp
        ${CMAKE_SOURCE_DIR}/tnl/huffmanStringProcessor.cpp
        ${CMAKE_SOURCE_DIR}/tnl/log.cpp
        ${CMAKE_SOURCE_DIR}/tnl/netBase.cpp
        ${CMAKE_SOURCE_DIR}/tnl/netConnection.cpp
        ${CMAKE_SOURCE_DIR}/tnl/netInterface.cpp
        ${CMAKE_SOURCE_DIR}/tnl/netObject.cpp
        ${CMAKE_SOURCE_DIR}/tnl/netStringTable.cpp
        ${CMAKE_SOURCE_DIR}/tnl/platform.cpp
        ${CMAKE_SOURCE_DIR}/tnl/rpc.cpp
        ${CMAKE_SOURCE_DIR}/tnl/tnlMethodDispatch.cpp
        ${CMAKE_SOURCE_DIR}/tnl/journal.cpp
        ${CMAKE_SOURCE_DIR}/tnl/udp.cpp
        ${CMAKE_SOURCE_DIR}/tnl/vector.cpp
    )

    # Add lib directory for linking
    link_directories(/c/msys64/mingw64/lib)

    # Create compatibility header for mycrypt.h -> tomcrypt.h
    file(WRITE "${CMAKE_BINARY_DIR}/mycrypt.h" [[
#include <tomcrypt.h>
]])

    add_library(tnl STATIC ${TNL_SOURCES})
    target_include_directories(tnl PUBLIC ${CMAKE_SOURCE_DIR}/tnl ${CMAKE_BINARY_DIR} /c/msys64/mingw64/include /mingw64/include)
    target_link_libraries(tnl tomcrypt)
    set(TNL_LIBRARY "tnl")
else()
    message(STATUS "Using pre-built TNL library: ${TNL_LIBRARY}")
endif()

if(TOMCRYPT_LIBRARY STREQUAL "TOMCRYPT_LIBRARY-NOTFOUND")
    message(STATUS "Tomcrypt library not found, continuing without it...")
    set(TOMCRYPT_LIBRARY "")
endif()

# Check for TNL headers in multiple locations
set(TNL_INCLUDE_DIR "")
check_include_file_cxx("tnl/tnl.h" HAS_TNL_HEADER_LOCAL)
if(HAS_TNL_HEADER_LOCAL)
    set(TNL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}")
endif()

if(NOT TNL_INCLUDE_DIR AND EXISTS "/home/geoffrey/tnl/tnl/tnl.h")
    set(TNL_INCLUDE_DIR "/home/geoffrey/tnl")
endif()

# Always use the source directory for TNL headers since we have the source
set(TNL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}")

include_directories(include ${TNL_INCLUDE_DIR} /c/msys64/mingw64/include)

# Graphics
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
include_directories(${PNG_INCLUDE_DIR} ${JPEG_INCLUDE_DIR})

# Flex and lemon
find_package(FLEX REQUIRED)
find_program(LEMON_EXECUTABLE lemon)
if(${LEMON_EXECUTABLE} STREQUAL "LEMON_EXECUTABLE-NOTFOUND")
    message(FATAL_ERROR "Lemon parser generator not found.")
endif()

# Collate all source files from subdirectories.
macro(append_subdir_files src_var tgt_var dirname)
    get_directory_property(holder DIRECTORY ${dirname} DEFINITION ${src_var})
    foreach(depfile ${holder})
       list(APPEND ${tgt_var} "${dirname}/${depfile}")
    endforeach()
endmacro()

macro(process_subdir dirname)
    add_subdirectory(${dirname})
    append_subdir_files(SRC_FILES LEGACY_SRC ${dirname})
endmacro()

include_directories(include ${TNL_INCLUDE_DIR})
add_definitions(-DTNL_LITTLE_ENDIAN)
add_definitions(-D_TNL_X86_)
add_definitions(-DFN_CDECL=)
add_definitions(-Di386)
add_definitions(-Dlinux)
add_definitions(-D__GNUC__)
add_definitions(-DQSORT_CALLBACK=)

process_subdir(engine)
process_subdir(audio)
process_subdir(video)
add_subdirectory(util)  # Add util first to get its SRC_FILES

# Build util as a static library
get_directory_property(UTIL_SRC_FILES DIRECTORY util DEFINITION SRC_FILES)
# Prepend util/ to source file names
set(UTIL_SRCS "")
foreach(src ${UTIL_SRC_FILES})
    list(APPEND UTIL_SRCS "util/${src}")
endforeach()
add_library(util STATIC ${UTIL_SRCS})
target_include_directories(util PUBLIC ${CMAKE_SOURCE_DIR}/include)

process_subdir(net)
process_subdir(interface)

# grammars is a special directory.
add_subdirectory(grammars)

add_executable(doomlegacy ${LEGACY_SRC})
target_link_libraries(doomlegacy grammars util
${SDL_LIBRARY} ${SDLMIXER_LIBRARY}
${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES}
${TNL_LIBRARY} ${TOMCRYPT_LIBRARY})

#============================================================================
# Test Suite Configuration
#============================================================================

enable_testing()

# Individual fixed_t test (Phase 1 foundation)
add_executable(test_fixed_t tests/unit/test_fixed_t.cpp)
target_include_directories(test_fixed_t PRIVATE include)
target_link_libraries(test_fixed_t ${SDL_LIBRARY})
add_test(NAME fixed_t_tests COMMAND test_fixed_t)

# Note: The run_tests target was removed because each unit test file has its own main()
# Individual unit tests are covered by their own executables below

# Integration tests for save/load serialization (excluding standalone tests)
file(GLOB INTEGRATION_TEST_SOURCES "tests/integration/test_save_load.cpp")
add_executable(test_save_load ${INTEGRATION_TEST_SOURCES})
target_include_directories(test_save_load PRIVATE include)
target_link_libraries(test_save_load ${SDL_LIBRARY})
add_test(NAME save_load_tests COMMAND test_save_load)

# Basic data serialization tests
add_executable(test_serialization tests/integration/test_serialization.cpp)
target_include_directories(test_serialization PRIVATE include)
target_link_libraries(test_serialization ${SDL_LIBRARY})
add_test(NAME serialization_tests COMMAND test_serialization)

# Individual unit tests
add_executable(test_mapinfo tests/unit/test_mapinfo.cpp)
target_include_directories(test_mapinfo PRIVATE include)
target_link_libraries(test_mapinfo util ${SDL_LIBRARY})
add_test(NAME mapinfo_tests COMMAND test_mapinfo)

add_executable(test_tnl_packets tests/unit/test_tnl_packets.cpp)
target_include_directories(test_tnl_packets PRIVATE include)
target_link_libraries(test_tnl_packets util ${SDL_LIBRARY})
add_test(NAME tnl_packet_tests COMMAND test_tnl_packets)

add_executable(test_actor tests/unit/test_actor.cpp)
target_include_directories(test_actor PRIVATE include)
target_link_libraries(test_actor util ${SDL_LIBRARY})
add_test(NAME actor_tests COMMAND test_actor)

add_executable(test_console tests/unit/test_console.cpp)
target_include_directories(test_console PRIVATE include)
target_link_libraries(test_console util ${SDL_LIBRARY})
add_test(NAME console_tests COMMAND test_console)

add_executable(test_vector tests/unit/test_vector.cpp)
target_include_directories(test_vector PRIVATE include)
target_link_libraries(test_vector util ${SDL_LIBRARY})
add_test(NAME vector_tests COMMAND test_vector)

# Demo Recording Tests
add_executable(test_demos tests/integration/test_demos.cpp)
target_include_directories(test_demos PRIVATE include)
target_link_libraries(test_demos util ${SDL_LIBRARY})
add_test(NAME demo_tests COMMAND test_demos)

# Network Integration Tests
add_executable(test_network_integration tests/integration/test_network_integration.cpp)
target_include_directories(test_network_integration PRIVATE include)
target_link_libraries(test_network_integration util ${SDL_LIBRARY})
add_test(NAME network_integration_tests COMMAND test_network_integration)

# Parity Tests (C++ vs C feature parity)
add_executable(test_parity tests/integration/test_parity.cpp)
target_include_directories(test_parity PRIVATE include ${TNL_INCLUDE_DIR})
target_link_libraries(test_parity util ${SDL_LIBRARY})
add_test(NAME parity_tests COMMAND test_parity)
