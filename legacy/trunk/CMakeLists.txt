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
option(USE_SOUND "Enable sound support" ON)
if(USE_SOUND)
    find_package(SDL_mixer REQUIRED)
    add_definitions("-DUSE_SOUND")
endif()
include_directories(${SDL_INCLUDE_DIR} ${SDL_MIXER_INCLUDE_DIR})
add_definitions("-DSDL")

# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# ENet for modern networking
option(USE_MODERN_NETWORKING "Use lnet for modern networking" ON)

if(USE_MODERN_NETWORKING)
    message(STATUS "Using lnet for modern networking")
    
    # Try to find ENet using CMake's find_package (works if ENet provides CMake config)
    # First attempt with QUIET - if found, it sets ENet_FOUND
    find_package(ENet 1.3 QUIET)
    
    if(ENet_FOUND)
        message(STATUS "Found ENet via CMake: ${ENet_VERSION}")
        include_directories(${ENET_INCLUDE_DIRS})
        set(ENET_LIBRARY ${ENET_LIBRARIES})
    else()
        # Try pkg-config as fallback (most common way to find ENet)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(PC_ENET QUIET libenet>=1.3)
            if(PC_ENET_FOUND)
                message(STATUS "Found ENet via pkg-config: ${PC_ENET_VERSION}")
                include_directories(${PC_ENET_INCLUDE_DIRS})
                set(ENET_LIBRARY ${PC_ENET_LIBRARIES})
                set(ENET_FOUND TRUE)
            endif()
        endif()
    endif()
    
    if(NOT ENET_FOUND)
        message(STATUS "ENet not found in system paths, checking common locations...")
        
        # Try platform-specific default locations
        if(WIN32)
            # Common Windows ENet install locations
            set(ENET_SEARCH_PATHS 
                "C:/enet-mingw"
                "C:/Program Files/enet"
                "${CMAKE_SOURCE_DIR}/../enet-mingw"
                "${CMAKE_SOURCE_DIR}/enet-mingw")
            find_path(ENET_INCLUDE_DIR NAMES enet/enet.h PATHS ${ENET_SEARCH_PATHS} PATH_SUFFIXES include)
            find_library(ENET_LIBRARY NAMES enet PATHS ${ENET_SEARCH_PATHS} PATH_SUFFIXES lib)
        else()
            # Linux/Unix common locations
            set(ENET_SEARCH_PATHS 
                "/usr"
                "/usr/local"
                "${CMAKE_SOURCE_DIR}/../enet"
                "${CMAKE_SOURCE_DIR}/enet")
            find_path(ENET_INCLUDE_DIR NAMES enet.h PATHS ${ENET_SEARCH_PATHS} PATH_SUFFIXES include)
            find_library(ENET_LIBRARY NAMES enet PATHS ${ENET_SEARCH_PATHS} PATH_SUFFIXES lib)
        endif()
        
        if(ENET_INCLUDE_DIR AND ENET_LIBRARY)
            message(STATUS "Found ENet at: include=${ENET_INCLUDE_DIR}, library=${ENET_LIBRARY}")
            include_directories(${ENET_INCLUDE_DIR})
            set(ENET_FOUND TRUE)
        endif()
    endif()
    
    if(NOT ENET_FOUND)
        message(FATAL_ERROR 
            "ENet 1.3 or later not found.\n"
            "Please install ENet or set ENET_INCLUDE_DIR and ENET_LIBRARY manually.\n"
            "On Windows, you can download from: https://github.com/lsalzman/enet/releases\n"
            "On Linux, install via: sudo apt-get install libenet-dev (Debian) or sudo yum install enet-devel (RHEL)")
    endif()
    
    # Create imported ENet target with proper Windows linking
    add_library(enet STATIC IMPORTED)
    set_target_properties(enet PROPERTIES 
        IMPORTED_LOCATION "${ENET_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ENET_INCLUDE_DIR}")
    
    # ENet on Windows requires ws2_32 (Winsock) and potentially winmm
    if(WIN32)
        set_target_properties(enet PROPERTIES 
            IMPORTED_LINK_DEPENDENT_LIBRARIES "ws2_32;winmm")
        message(STATUS "ENet Windows dependencies: ws2_32, winmm")
    endif()
else()
    message(STATUS "Using legacy TNL networking")
    # TNL (legacy)
    if(EXISTS "/home/geoffrey/tnl/lib/libtnl.a")
        set(TNL_LIBRARY "/home/geoffrey/tnl/lib/libtnl.a")
    else()
        find_library(TNL_LIBRARY tnl)
    endif()
    if(${TNL_LIBRARY} STREQUAL "TNL_LIBRARY-NOTFOUND")
        message(FATAL_ERROR "TNL library not found.")
    endif()
    if(EXISTS "/home/geoffrey/tnl/lib/libtomcrypt.a")
        set(TOMCRYPT_LIBRARY "/home/geoffrey/tnl/lib/libtomcrypt.a")
    else()
        find_library(TOMCRYPT_LIBRARY tomcrypt)
    endif()
    if(${TOMCRYPT_LIBRARY} STREQUAL "TOMCRYPT_LIBRARY-NOTFOUND")
        message(FATAL_ERROR "Tomcrypt library not found.")
    endif()
    
    check_include_file_cxx("tnl/tnl.h" HAS_TNL_HEADER)
    if(NOT ${HAS_TNL_HEADER})
        if(EXISTS "/home/geoffrey/tnl/tnl/tnl.h")
            include_directories(/home/geoffrey/tnl)
        else()
            message(FATAL_ERROR "TNL headers not found.")
        endif()
    endif()
endif()

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

include_directories(include)
include_directories(tnl)
# include_directories(lnet)  # disabled: directory not present

if(NOT USE_MODERN_NETWORKING)
    include_directories(/home/geoffrey/tnl/include)
    add_definitions(-DTNL_LITTLE_ENDIAN)
    add_definitions(-D_TNL_X86_)
endif()

add_definitions(-DFN_CDECL=)
add_definitions(-Di386)
add_definitions(-Dlinux)
add_definitions(-D__GNUC__)
add_definitions(-DQSORT_CALLBACK=)

process_subdir(engine)
if(USE_SOUND)
    process_subdir(audio)
endif()
process_subdir(video)
add_subdirectory(util)  # Add util first to get its SRC_FILES
# add_subdirectory(lnet)  # Add lnet for modern networking - disabled: directory not present

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

add_executable(d2h tools/d2h.cpp)
add_executable(wadtool tools/wadtool.cpp util/md5.cpp)

add_executable(doomlegacy ${LEGACY_SRC})
if(WIN32)
  target_link_libraries(doomlegacy grammars util
  ${SDL_LIBRARY} ${SDLMIXER_LIBRARY}
  ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES}
  ws2_32 mswsock winmm)
else()
  target_link_libraries(doomlegacy grammars util
  ${SDL_LIBRARY} ${SDLMIXER_LIBRARY}
  ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES}
  pthread)
endif()

if(USE_MODERN_NETWORKING)
    target_link_libraries(doomlegacy lnet)
else()
    target_link_libraries(doomlegacy ${TNL_LIBRARY} ${TOMCRYPT_LIBRARY})
endif()

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

# Save/Load Integration Tests
add_executable(test_save_load_unit tests/integration/test_save_load.cpp)
target_include_directories(test_save_load_unit PRIVATE include)
target_link_libraries(test_save_load_unit util ${SDL_LIBRARY})
add_test(NAME save_load_unit_tests COMMAND test_save_load_unit)

# Basic data serialization tests
add_executable(test_serialization tests/integration/test_serialization.cpp)
target_include_directories(test_serialization PRIVATE include)
target_link_libraries(test_serialization ${SDL_LIBRARY})
add_test(NAME serialization_tests COMMAND test_serialization)

# MapInfo Unit Tests - source file not present
# add_executable(test_mapinfo_constants tests/unit/test_mapinfo_constants.cpp)
# target_include_directories(test_mapinfo_constants PRIVATE include)
# target_link_libraries(test_mapinfo_constants util ${SDL_LIBRARY})
# add_test(NAME mapinfo_constants_tests COMMAND test_mapinfo_constants)

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

# Demo Integration Tests
add_executable(test_demos_integration tests/integration/test_demos.cpp)
target_include_directories(test_demos_integration PRIVATE include)
target_link_libraries(test_demos_integration util ${SDL_LIBRARY})
add_test(NAME demos_integration_tests COMMAND test_demos_integration)

# Network Integration Tests
add_executable(test_network_integration tests/integration/test_network_integration.cpp)
target_include_directories(test_network_integration PRIVATE include)
target_link_libraries(test_network_integration util ${SDL_LIBRARY})
add_test(NAME network_integration_tests COMMAND test_network_integration)

# Parity Tests (C++ vs C feature parity)
add_executable(test_parity tests/integration/test_parity.cpp)
target_include_directories(test_parity PRIVATE include /home/geoffrey/tnl/include)
target_link_libraries(test_parity util ${SDL_LIBRARY})
add_test(NAME parity_tests COMMAND test_parity)

# Console Colors Unit Tests - source file not present
# add_executable(test_console_colors tests/unit/test_console_colors.cpp)
# target_include_directories(test_console_colors PRIVATE include)
# target_link_libraries(test_console_colors util ${SDL_LIBRARY})
# add_test(NAME console_colors_tests COMMAND test_console_colors)

# Save/Load Unit Tests (renamed to avoid conflict)
