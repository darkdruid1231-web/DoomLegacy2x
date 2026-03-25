@echo off
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;C:\Users\Geoffrey\codeql;%PATH%
cd /d C:\Users\Geoffrey\Desktop\Doom\doomlegacy\doomlegacy-svn\legacy\trunk
if exist build-codeql rmdir /s /q build-codeql
mkdir build-codeql
cmake -G Ninja -B build-codeql -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_MAKE_PROGRAM=C:\msys64\mingw64\bin\ninja.exe ^
  -DBISON_EXECUTABLE=C:\msys64\usr\bin\bison.exe ^
  -DFLEX_EXECUTABLE=C:\msys64\usr\bin\flex.exe ^
  -DLEMON_EXECUTABLE=C:\msys64\usr\bin\lemon.exe .
cmake --build build-codeql --parallel
