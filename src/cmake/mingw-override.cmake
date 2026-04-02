# CMake hook to fix MinGW include paths after project() is called
# This removes the Linux /usr/include from CMake-generated include paths

# Get all include directories and filter out /usr/include
get_directory_property(DIR_INCLUDES INCLUDE_DIRECTORIES)

foreach(INCLUDE_DIR ${DIR_INCLUDES})
    if("${INCLUDE_DIR}" STREQUAL "/usr/include" OR "${INCLUDE_DIR}" STREQUAL "/usr/local/include")
        message(STATUS "Removing system include: ${INCLUDE_DIR}")
        remove_from_directory_info(INCLUDE_DIRECTORIES "${INCLUDE_DIR}")
    endif()
endforeach()

# Also fix the compiler flags if they somehow got /usr/include added
# We'll handle this in the build step by creating wrapper scripts
