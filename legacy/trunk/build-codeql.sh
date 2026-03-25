#!/bin/bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:$HOME/codeql:$PATH"
cd "C:/Users/Geoffrey/Desktop/Doom/doomlegacy/doomlegacy-svn/legacy/trunk"
mkdir -p build-codeql
cmake -G Ninja -B build-codeql -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_MAKE_PROGRAM="/c/msys64/mingw64/bin/ninja.exe" \
  -DBISON_EXECUTABLE="/c/msys64/usr/bin/bison.exe" \
  -DFLEX_EXECUTABLE="/c/msys64/usr/bin/flex.exe" \
  -DLEMON_EXECUTABLE="/c/msys64/usr/bin/lemon.exe" .
ninja -C build-codeql
