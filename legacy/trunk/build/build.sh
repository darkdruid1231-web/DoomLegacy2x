#!/bin/bash

# Navigate to build directory
cd /c/Users/Geoffrey/Desktop/Doom/doomlegacy/doomlegacy-svn/legacy/trunk/build

# Configure with CMake
cmake -DFLEX_EXECUTABLE=/usr/bin/flex -DBISON_EXECUTABLE=/usr/bin/bison -DLEMON_EXECUTABLE=/usr/bin/lemon -DENET_INCLUDE_DIR=/mingw64/include -DENET_LIBRARY=/mingw64/lib/libenet.a ..

# Rebuild only the main executable (since most object files are already built)
mingw32-make util
mingw32-make doomlegacy
