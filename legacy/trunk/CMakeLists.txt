cmake_minimum_required(VERSION 3.16)
project(doomlegacy C CXX)

# Global compile options
# -w: suppress all warnings (including GCC 15 / MinGW intrinsics redefinition)
# -fpermissive: allow implicit conversions, etc.
# Fix GCC 13+ / MinGW-w64 intrinsics conflict: define these BEFORE any includes
# to prevent GCC's xsaveintrin.h from declaring conflicting signed versions
add_compile_definitions(__XSAVEINTRIN_H _XGETBV_DEFINED)

add_compile_options(-w -fpermissive)

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

# Modern compiler detection - CMAKE_COMPILER_IS_GNUCXX was removed in CMake 3.6+
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Check for C++11 support
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(NOT COMPILER_SUPPORTS_CXX11)
        message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER_ID} has no C++11 support. Please use a newer GCC version.")
    endif()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    # Keep -fpermissive and -w for backward compatibility with legacy code
    add_compile_options(-fpermissive -w)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    # Clang compatibility
    add_compile_options(-w)
else()
    message(FATAL_ERROR "Only GNU C++ and Clang are supported at the moment.")
endif()

# External libraries. First SDL.
# Use pkg-config for SDL2 (more reliable across platforms)
option(USE_SDL2 "Enable SDL2 support" ON)

if(USE_SDL2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    option(USE_SOUND "Enable sound support" ON)
    if(USE_SOUND)
        pkg_check_modules(SDL2MIXER sdl2mixer)
        if(NOT SDL2MIXER_FOUND)
            message(STATUS "SDL2_mixer not found, disabling sound")
            set(USE_SOUND OFF)
        endif()
    endif()
    include_directories(${SDL2_INCLUDE_DIRS} ${SDL2MIXER_INCLUDE_DIRS})
else()
    message(STATUS "SDL2 disabled, using SDL1")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL REQUIRED sdl)
    include_directories(${SDL_INCLUDE_DIRS})
    add_compile_definitions(SDL)
    # Set SDL2 variables to SDL for compatibility
    set(SDL2_LIBRARIES ${SDL_LIBRARIES})
    set(SDL2MIXER_LIBRARIES "")
endif()
# SDL definition added via target_compile_definitions below

# Zlib for compression (used by m_archive.cpp)
find_package(ZLIB)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    set(EXTRA_LIBS ${EXTRA_LIBS} ${ZLIB_LIBRARIES})
else()
    message(STATUS "ZLIB not found, m_archive.cpp compression may fail")
endif()

# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# Option to disable networking entirely (for platforms without networking support)
option(USE_NETWORKING "Enable networking support" ON)

# Option to disable image support (for Windows cross-compilation without image libs)
option(USE_IMAGE_SUPPORT "Enable image (PNG/JPEG) support" ON)

# ENet for modern networking
if(USE_NETWORKING)
    # Include TNL stub headers for compatibility only when networking is enabled
    include_directories(${CMAKE_SOURCE_DIR}/include_stubs)
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
    # Skip stub headers when networking is disabled
    
    # Skip TNL library finding when networking is disabled (Windows cross-compile)
    if(USE_NETWORKING)
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
    endif()  # USE_NETWORKING
    
    check_include_file_cxx("tnl/tnl.h" HAS_TNL_HEADER)
    if(NOT ${HAS_TNL_HEADER})
        if(EXISTS "/home/geoffrey/tnl/tnl/tnl.h")
            include_directories(/home/geoffrey/tnl)
        else()
            message(FATAL_ERROR "TNL headers not found.")
        endif()
    endif()
    endif()  # USE_MODERN_NETWORKING
endif()  # USE_NETWORKING

# Graphics - PNG/JPEG support (can be disabled for Windows cross-compilation)
if(USE_IMAGE_SUPPORT)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
include_directories(${PNG_INCLUDE_DIR} ${JPEG_INCLUDE_DIR})
else()
message(STATUS "Image support disabled (USE_IMAGE_SUPPORT=OFF)")
endif()

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
include_directories(include/core)
include_directories(tnl)
# include_directories(lnet)  # disabled: directory not present

if(NOT USE_MODERN_NETWORKING)
    include_directories(/home/geoffrey/tnl/include)
    # TNL definitions added via target_compile_definitions below
endif()

# Platform-specific definitions - now using target_compile_definitions below
# (removed add_definitions here)

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

# Apply platform-specific definitions to util (required for TNL headers)
target_compile_definitions(util PRIVATE
    FN_CDECL=
    i386
    linux
    __GNUC__
    QSORT_CALLBACK=
)

if(NOT USE_MODERN_NETWORKING)
    target_compile_definitions(util PRIVATE
        TNL_LITTLE_ENDIAN
        _TNL_X86_
    )
endif()

process_subdir(net)
process_subdir(interface)

# grammars is a special directory.
add_subdirectory(grammars)

add_executable(d2h tools/d2h.cpp)
add_executable(wadtool tools/wadtool.cpp util/md5.cpp)

add_executable(doomlegacy ${LEGACY_SRC})

# Modern target-based definitions (replacing deprecated add_definitions)
# NOTE: Do NOT redefine __GNUC__ here — it's a compiler built-in and redefining it
# to a different value (e.g., 1) breaks version checks like:
#   #if __GNUC__ >= 4  →  #if 1 >= 4  (false)
# which then uses broken legacy code paths.
target_compile_definitions(doomlegacy PRIVATE
    FN_CDECL=
    QSORT_CALLBACK=
)

# _USE_MATH_DEFINES: required on Windows (MSVC / MinGW / MSYS2) to expose M_PI
# and other POSIX math constants from <math.h>/<cmath>.  Must be defined before
# those headers are included — passing it as a compile definition guarantees that.
if(WIN32)
    target_compile_definitions(doomlegacy PRIVATE _USE_MATH_DEFINES)
endif()

# SDL define only when SDL2 is actually enabled (otherwise doomtype.h tries to
# include SDL_types.h which won't be in the include path)
if(USE_SDL2)
    target_compile_definitions(doomlegacy PRIVATE SDL)
endif()

# Platform-specific defines: only set linux/i386 on actual Linux builds
# These must NOT be set on Windows (MSYS2 / MinGW cross-compile) because they
# cause doomtype.h and platform detection code to assume a Linux environment
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(doomlegacy PRIVATE linux i386)
endif()

# Conditional definitions
if(USE_SOUND)
    target_compile_definitions(doomlegacy PRIVATE USE_SOUND)
endif()

if(NOT USE_MODERN_NETWORKING)
    target_compile_definitions(doomlegacy PRIVATE
        TNL_LITTLE_ENDIAN
        _TNL_X86_
    )
endif()

# Build image libraries list conditionally
set(IMAGE_LIBRARIES "")
if(USE_IMAGE_SUPPORT)
    set(IMAGE_LIBRARIES ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
endif()

if(WIN32)
  target_link_libraries(doomlegacy grammars util
  ${SDL2_LIBRARIES} ${SDL2MIXER_LIBRARIES}
  ${OPENGL_LIBRARIES} ${IMAGE_LIBRARIES}
  ws2_32 mswsock winmm)
else()
  target_link_libraries(doomlegacy grammars util
  ${SDL2_LIBRARIES} ${SDL2MIXER_LIBRARIES}
  ${OPENGL_LIBRARIES} ${IMAGE_LIBRARIES}
  pthread)
endif()

if(USE_MODERN_NETWORKING)
    target_link_libraries(doomlegacy lnet)
else()
    target_link_libraries(doomlegacy ${TNL_LIBRARY} ${TOMCRYPT_LIBRARY})
endif()

#============================================================================
# Catch2 Test Framework
#============================================================================

# Try to find system Catch2 first
find_package(Catch2 QUIET)

if(NOT Catch2_FOUND)
    # Fetch Catch2 from GitHub if not found
    include(FetchContent)
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
    )
    # FetchContent_MakeAvailable sets up the target
    FetchContent_MakeAvailable(catch2)
    message(STATUS "Catch2 fetched via FetchContent")
else()
    message(STATUS "Catch2 found: ${Catch2_VERSION}")
endif()

#============================================================================
# Test Suite Configuration
#============================================================================

enable_testing()

# Individual fixed_t test (legacy framework)
add_executable(test_fixed_t tests/unit/test_fixed_t.cpp)
target_include_directories(test_fixed_t PRIVATE include)
if(USE_SDL2)
    target_link_libraries(test_fixed_t ${SDL2_LIBRARIES})
endif()
add_test(NAME fixed_t_tests COMMAND test_fixed_t)

# Individual fixed_t test using Catch2 (Phase 1 - new framework)
add_executable(test_fixed_catch2 tests/unit/test_fixed_catch2.cpp)
target_include_directories(test_fixed_catch2 PRIVATE include)
target_link_libraries(test_fixed_catch2 PRIVATE Catch2::Catch2WithMain)
add_test(NAME fixed_t_catch2_tests COMMAND test_fixed_catch2)

# Note: The run_tests target was removed because each unit test file has its own main()
# Individual unit tests are covered by their own executables below

# Save/Load Integration Tests
add_executable(test_save_load_unit tests/integration/test_save_load.cpp)
target_include_directories(test_save_load_unit PRIVATE include)
target_link_libraries(test_save_load_unit util ${SDL2_LIBRARIES})
add_test(NAME save_load_unit_tests COMMAND test_save_load_unit)

# Basic data serialization tests
add_executable(test_serialization tests/integration/test_serialization.cpp)
target_include_directories(test_serialization PRIVATE include)
target_link_libraries(test_serialization ${SDL2_LIBRARIES})
add_test(NAME serialization_tests COMMAND test_serialization)

# MapInfo Unit Tests - source file not present
# add_executable(test_mapinfo_constants tests/unit/test_mapinfo_constants.cpp)
# target_include_directories(test_mapinfo_constants PRIVATE include)
# target_link_libraries(test_mapinfo_constants util ${SDL2_LIBRARIES})
# add_test(NAME mapinfo_constants_tests COMMAND test_mapinfo_constants)

add_executable(test_tnl_packets tests/unit/test_tnl_packets.cpp)
target_include_directories(test_tnl_packets PRIVATE include)
target_link_libraries(test_tnl_packets util ${SDL2_LIBRARIES})
add_test(NAME tnl_packet_tests COMMAND test_tnl_packets)

add_executable(test_actor tests/unit/test_actor.cpp)
target_include_directories(test_actor PRIVATE include)
target_link_libraries(test_actor util ${SDL2_LIBRARIES})
add_test(NAME actor_tests COMMAND test_actor)

# Component pattern tests using Catch2 (dependency injection, thinkable components)
add_executable(test_components tests/unit/test_components.cpp)
target_include_directories(test_components PRIVATE include)
target_link_libraries(test_components PRIVATE Catch2::Catch2WithMain)
add_test(NAME component_tests COMMAND test_components)

add_executable(test_console tests/unit/test_console.cpp)
target_include_directories(test_console PRIVATE include)
target_link_libraries(test_console util ${SDL2_LIBRARIES})
add_test(NAME console_tests COMMAND test_console)

add_executable(test_vector tests/unit/test_vector.cpp)
target_include_directories(test_vector PRIVATE include)
target_link_libraries(test_vector util ${SDL2_LIBRARIES})
add_test(NAME vector_tests COMMAND test_vector)

# Demo Integration Tests
add_executable(test_demos_integration tests/integration/test_demos.cpp)
target_include_directories(test_demos_integration PRIVATE include)
target_link_libraries(test_demos_integration util ${SDL2_LIBRARIES})
add_test(NAME demos_integration_tests COMMAND test_demos_integration)

# Network Integration Tests
add_executable(test_network_integration tests/integration/test_network_integration.cpp)
target_include_directories(test_network_integration PRIVATE include)
target_link_libraries(test_network_integration util ${SDL2_LIBRARIES})
add_test(NAME network_integration_tests COMMAND test_network_integration)

# Parity Tests (C++ vs C feature parity)
add_executable(test_parity tests/integration/test_parity.cpp)
target_include_directories(test_parity PRIVATE include /home/geoffrey/tnl/include)
target_link_libraries(test_parity util ${SDL2_LIBRARIES})
add_test(NAME parity_tests COMMAND test_parity)

# Console Colors Unit Tests - source file not present
# add_executable(test_console_colors tests/unit/test_console_colors.cpp)
# target_include_directories(test_console_colors PRIVATE include)
# target_link_libraries(test_console_colors util ${SDL2_LIBRARIES})
# add_test(NAME console_colors_tests COMMAND test_console_colors)

# Save/Load Unit Tests (renamed to avoid conflict)
