# util_base - Pure math utilities with no external dependencies
#
# These files have minimal dependencies (doomtype.h only) and can be
# tested without linking against the full util library which has
# complex dependencies on interface/ (I_Error, CONS_Printf).

set(UTIL_BASE_SOURCES
    m_fixed.cpp
    m_swap.cpp
    tables.cpp
)

add_library(util_base STATIC ${UTIL_BASE_SOURCES})

target_include_directories(util_base PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
)

# Exclude from ALLulli building with the rest of util since these are tested separately
get_property(UTIL_BASE_GLOBAL TARGET util_base PROPERTY IMPORTED)
if(NOT UTIL_BASE_GLOBAL)
    set_target_properties(util_base PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
