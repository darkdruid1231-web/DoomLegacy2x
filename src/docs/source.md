# Doom Legacy in C++

Branched off from version 1.40.

## Main ideas behind the conversion

- Extendable:
  - Real C++ code, classes, encapsulation, inheritance, polymorphism.
  - Logical, easy to understand source file structure.
  - Well documented code, standard coding style.

- Easily portable:
  - Multimedia API always SDL 1.2.5+ if possible.
  - Graphics rendered using OpenGL 1.2 through SDL.
  - Audio maybe through OpenAL?
  - Absolute minimum of platform-specific `#ifdef`s (only used to sort out standard library differences).

- Utilizes existing code:
  - Data structures constructed using STL 3.0.
  - Use existing, efficient, portable LGPL libraries whenever possible (libpng, zlib etc.).

---

## Coding conventions

- Use Emacs-style indentation. Better yet, use Emacs to write your code.
- Fully document all files, classes, data members, methods and non-class functions using Doxygen syntax.
- If you must commit something that is broken, leave a comment including the exact word `FIXME` and a description of what should be fixed. For example:
  ```
  // FIXME : here it crashes because int *p is sometimes NULL
  ```
  This keyword can be then grepped by other developers.
- If something in your code isn't exactly broken, but you think it could be improved, you can use the word `TODO` in the same way:
  ```
  // TODO : replace the bubble sort with STL introsort
  ```

---

## Documentation

Basic Doom/Boom/Heretic/Hexen docs:

- Matt Fell's [Unofficial Doom specs v1.666](http://www.gamers.org/dEngine/doom/spec/uds.1666.txt)
- Dr Sleep's [Unofficial Heretic specs v1.0](http://www.hut.fi/u/vberghol/doom/hereticspec10.txt)
- Ben Morris's [Hexen specs v0.9](http://www.hut.fi/u/vberghol/doom/hexenspec09.txt)
- Team TNT's [Boom reference v1.3](boomref.html)

C++ guides:

- A good [C++ tutorial](http://www.cplusplus.com/)
- SGI's [STL documentation](http://www.sgi.com/tech/stl/)

---

## Programming tools and libraries

- [MinGW](http://www.mingw.org/), Minimalist GNU for Windows. Gnu Compiler Collection (GCC) for Win32.
- [MSYS](http://www.mingw.org/msys.shtml), Minimal POSIX system for Win32. Needed to properly run Makefiles and configuration tools. Includes a real shell (BASH) and utilities like grep and sed (of utmost importance).
- [SDL](http://www.libsdl.org/), Simple DirectMedia Layer. A multiplatform multimedia library.
- [SDL_mixer](http://www.libsdl.org/projects/SDL_mixer/), a multichannel mixer library based on SDL.
- [zlib](http://www.zlib.org/), a free compression library.
- [OpenTNL](http://www.opentnl.org/), a free game networking library based on the Torque Network Library.
- [Mesa 3D](http://www.mesa3d.org/), a free OpenGL implementation.
  Note: Windows includes OpenGL runtime DLLs, all you need are the header files and import libraries used when compiling and linking. They should be included in the MinGW standard distribution.
- [Doxygen](http://www.doxygen.org/), an automatic source documentation system.
