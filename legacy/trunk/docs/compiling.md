# How to compile Doom Legacy (SDL2 version)

In order to compile Doom Legacy with SDL2 you'll need to have several libraries installed on your system.
Make sure you install the **developer** packages of the libraries, which include both the runtime libraries (DLLs) and the stuff required during compilation (header files and import libraries).

> **NOTE:** Most Linux distributions offer these libraries in the form of precompiled packages. On Ubuntu/Debian, you can install most dependencies with:
> ```
> sudo apt install cmake g++ libsdl2-dev libsdl2-mixer-dev libglew-dev libpng-dev zlib1g-dev libjpeg-dev
> ```

## Required libraries

| Library | Version | Ubuntu/Debian package | Description |
|---|---|---|---|
| [SDL2](https://www.libsdl.org/download-2.0.php) | 2.0.10+ | libsdl2-dev | Simple DirectMedia Layer. A multiplatform multimedia library. |
| [SDL2_mixer](https://github.com/libsdl-org/SDL_mixer/releases) | 2.0.4+ | libsdl2-mixer-dev | A multichannel mixer library based on SDL2. |
| OpenGL | 1.3+ | (several) | The standard cross-platform graphics library, usually comes with the OS. GLSL shaders require OpenGL 2.0+. |
| [zlib](http://www.zlib.org/) | 1.1.4+ | [zlib1g-dev](http://packages.ubuntu.com/zlib1g-dev) | A free compression library. |
| [libpng](http://www.libpng.org/pub/png/libpng.html) | 1.2.5+ | [libpng12-dev](http://packages.ubuntu.com/libpng12-dev) | The official PNG reference library. |
| [libjpeg](http://www.ijg.org/) | 6b | libjpeg-dev | The unofficial JPEG reference library. |
| [GLEW](https://glew.sourceforge.io/) | 2.1+ | libglew-dev | OpenGL Extension Wrangler Library. |

## Required programs

- [GCC](http://gcc.gnu.org/) 4+, the Gnu Compiler Collection which, among other things, is a free C/C++ compiler.
  Linux systems most likely already have it installed.
  Windows users should install [MSYS2](https://www.msys2.org/), which includes MinGW-w64 GCC toolchain.
- [GNU flex](http://flex.sourceforge.net/), a scanner generator. (Ubuntu package [flex](http://packages.ubuntu.com/flex))
- (optional) [Doxygen](http://www.doxygen.org/), an automatic source documentation system.

Download the [Doom Legacy source](http://sourceforge.net/projects/doomlegacy/). You can either get the source package or, for the latest code snapshot, checkout the **legacy/trunk/** directory from the [Subversion](http://subversion.apache.org/) repository:

```
svn co https://doomlegacy.svn.sourceforge.net/svnroot/doomlegacy/legacy/trunk some_local_dir
```

From now on, your local copy of this directory will be referred to as **TRUNK**.

---

## Compiling with CMake on Linux

CMake is the primary build system for Doom Legacy. The build uses a TNL stub by default, so network multiplayer requires additional setup.

You'll get [CMake](https://www.cmake.org) from your Linux distribution.

1. Go to your source root (`legacy/trunk/`).
2. `mkdir build && cd build`
3. `cmake -DCMAKE_BUILD_TYPE=Release ..`
4. `cmake --build . --config Release`

---

## Compiling on Windows with MSYS2/MinGW

Doom Legacy can be built on Windows using MSYS2 with MinGW-w64. This is the recommended approach for Windows builds. The build uses the [Ninja](https://ninja-build.org/) generator — do **not** use `-G "MinGW Makefiles"`, as that generator has a broken incremental-build dependency tracker on Windows and will silently skip recompilation of changed files.

### Prerequisites

- [MSYS2](https://www.msys2.org/) — install MSYS2 and open the **MSYS2 MinGW 64-bit** shell (not the plain MSYS shell).
- Install the required development packages:
  ```
  pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-glew mingw-w64-x86_64-libpng mingw-w64-x86_64-zlib
  ```
- Ensure your Doom WAD file (`doom.wad`) is available.

### Build steps

1. Open the **MSYS2 MinGW 64-bit** terminal.
2. Navigate to the source directory:
   ```
   cd /c/path/to/doomlegacy/legacy/trunk
   ```
3. Create and enter the build directory:
   ```
   mkdir -p build && cd build
   ```
4. Configure with CMake (Ninja generator):
   ```
   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
   ```
5. Build the project:
   ```
   ninja
   ```

For subsequent incremental builds, just run `ninja` again from the build directory — it will recompile only what changed.

### Running the game

After building, the executable will be in the build directory. Run with:

```
./doomlegacy.exe -iwad path/to/doom.wad
```

---

## Compiling on Windows with Visual Studio

Doom Legacy can also be built on Windows using Visual Studio 2022.

### Prerequisites

- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) with C++ workload
- [CMake](https://cmake.org/download/) 3.16 or later
- [SDL2](https://www.libsdl.org/download-2.0.php) development libraries
- [SDL2_mixer](https://github.com/libsdl-org/SDL_mixer/releases) development libraries

### Build steps

1. Open "Developer Command Prompt for VS2022" or "x64 Native Tools Command Prompt".
2. Navigate to the source directory and create build directory:
   ```
   cd legacy\trunk
   mkdir build
   cd build
   ```
3. Configure with CMake:
   ```
   cmake -G "Visual Studio 17 2022" -A x64 ..
   ```
4. Build the project:
   ```
   cmake --build . --config Release
   ```

The executable will be at **build\Release\doomlegacy.exe**.
