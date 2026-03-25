@echo off
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;C:\Users\Geoffrey\codeql;%PATH%
set CC=gcc
set CXX=g++
cd /d C:\Users\Geoffrey\Desktop\Doom\doomlegacy\doomlegacy-svn\legacy_one\trunk
if not exist objs\nul mkdir objs
if not exist bin\nul mkdir bin
if not exist dep\nul mkdir dep
if not exist tools\objs\nul mkdir tools\objs
if not exist tools\bin\nul mkdir tools\bin

REM Build fixdep tool first
if not exist tools\bin\fixdep.exe (
    gcc -O4 -Wall -c tools\fixdep.c -o tools\objs\fixdep.o
    gcc -Xlinker --warn-common tools\objs\fixdep.o -o tools\bin\fixdep.exe
)

REM Compile C files from src/
cd src
for %%f in (*.c) do (
    echo Compiling C: %%f
    gcc -Wall -O3 -fomit-frame-pointer -ffast-math -fno-strict-aliasing -DWIN32 -DHWRENDER -DSMIF_SDL -DHAVE_MIXER -DCDMUS -IC:/msys64/mingw64/include/SDL -D_GNU_SOURCE=1 -Dmain=SDL_main -I. -c %%f -o ..\\objs\\%%~nf.o
)

REM Compile C++ files from src/
for %%f in (*.cpp) do (
    echo Compiling C++: %%f
    g++ -Wall -O3 -fomit-frame-pointer -ffast-math -fno-strict-aliasing -DWIN32 -DHWRENDER -DSMIF_SDL -DHAVE_MIXER -DCDMUS -IC:/msys64/mingw64/include/SDL -D_GNU_SOURCE=1 -Dmain=SDL_main -I. -c %%f -o ..\\objs\\%%~nf.o
)
