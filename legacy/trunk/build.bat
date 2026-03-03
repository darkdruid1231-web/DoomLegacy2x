@echo off
REM Doom Legacy Windows Build Script
REM This script builds Doom Legacy on Windows using CMake and Visual Studio

echo Doom Legacy Windows Build Script
echo =================================

REM Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: CMake not found. Please install CMake from https://cmake.org/download/
    echo Make sure cmake.exe is in your PATH
    pause
    exit /b 1
)

REM Check for Visual Studio
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio C++ compiler not found.
    echo Please run this from "Developer Command Prompt for VS" or install Visual Studio with C++ workload
    pause
    exit /b 1
)

REM Set build directory
set BUILD_DIR=build
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Check for SDL2
echo Checking for SDL2...
where sdl2-config >nul 2>nul
if %errorlevel% neq 0 (
    REM Try common SDL2 installation paths
    if exist "C:\SDL2\bin\sdl2-config.exe" (
        set PATH=C:\SDL2\bin;%PATH%
    ) else if exist "C:\Program Files\SDL2\bin\sdl2-config.exe" (
        set PATH=C:\Program Files\SDL2\bin;%PATH%
    ) else (
        echo WARNING: SDL2 not found in PATH or common locations.
        echo Please install SDL2 development libraries from https://www.libsdl.org/download-2.0.php
        echo Or set SDL2DIR environment variable to your SDL2 installation directory
        echo Continuing anyway - CMake may find it...
    )
)

REM Configure with CMake
echo Configuring build with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

REM Build the project
echo Building Doom Legacy...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable location: %BUILD_DIR%\Release\doomlegacy.exe
echo.
echo To run the game:
echo   cd %BUILD_DIR%\Release
echo   doomlegacy.exe -iwad path\to\doom.wad
echo.
pause