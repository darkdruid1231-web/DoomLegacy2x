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
find_library(TNL_LIBRARY tnl)
if(${TNL_LIBRARY} STREQUAL "TNL_LIBRARY-NOTFOUND")
    message(FATAL_ERROR "TNL library not found.")
endif()
find_library(TOMCRYPT_LIBRARY tomcrypt)
if(${TOMCRYPT_LIBRARY} STREQUAL "TOMCRYPT_LIBRARY-NOTFOUND")
    message(FATAL_ERROR "Tomcrypt library not found.")
endif()

check_include_file_cxx("tnl/tnl.h" HAS_TNL_HEADER)
#if("${HAS_TNL_HEADER}" STREQUAL "")
#    message(FATAL_ERROR "TNL headers not found.")
#endif()

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

include_directories(include /home/geoffrey/tnl/include)
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
process_subdir(util)
process_subdir(net)
process_subdir(interface)

# grammars is a special directory.
add_subdirectory(grammars)


add_executable(d2h tools/d2h.cpp)
add_executable(wadtool tools/wadtool.cpp util/md5.cpp)
#add_executable(convert_deh tools/convert_deh.cpp util/mnemonics.cpp)

add_executable(doomlegacy ${LEGACY_SRC})
target_link_libraries(doomlegacy grammars
${SDL_LIBRARY} ${SDLMIXER_LIBRARY}
${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES}
${TNL_LIBRARY} ${TOMCRYPT_LIBRARY})

#============================================================================
# Test Suite Configuration
#============================================================================

enable_testing()

# Unit tests
file(GLOB UNIT_TEST_SOURCES "tests/unit/*.cpp")
add_executable(run_tests ${UNIT_TEST_SOURCES})
target_include_directories(run_tests PRIVATE include)
target_link_libraries(run_tests ${SDL_LIBRARY})
add_test(NAME unit_tests COMMAND run_tests)

# Optional: Integration tests (when implemented)
# add_executable(run_integration_tests tests/integration/*.cpp)
# target_include_directories(run_integration_tests PRIVATE include)
# target_link_libraries(run_integration_tests doomlegacy ${SDL_LIBRARY} ${TNL_LIBRARY})
# add_test(NAME integration_tests COMMAND run_integration_tests)
