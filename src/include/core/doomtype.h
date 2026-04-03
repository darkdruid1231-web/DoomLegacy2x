// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: doomtype.h 705 2010-07-13 20:33:06Z jussip $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2007 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Basic typedefs and platform-dependent #defines

#ifndef doomtype_h
#define doomtype_h 1

// Standard library differences
#ifdef __WIN32__
// Fix GCC 13+ / MinGW-w64 header conflicts: prevent GCC from declaring its own _xgetbv and
// __cpuidex Let MinGW-w64's intrin-impl.h provide the compatible versions
#ifdef __MINGW32__
#define __XSAVEINTRIN_H
#define _XGETBV_DEFINED
#define __CPUID_H
#define _CPUID_H_INCLUDED
#endif

// Don't include windows.h early - causes intrinsics conflict with GCC 13+
// Provide portable Win32 type replacements only for non-Windows builds
#ifndef _WIN32
#include <stdint.h>
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int BOOL;
#endif
// On Windows, let MinGW provide these types

#define ASMCALL __cdecl
#else
#define ASMCALL
#endif

#ifdef __APPLE_CC__
#define __MACOS__
#define DEBUG_LOG
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

#if defined(__MSC__) || defined(__OS2__) // Microsoft VisualC++
#define strncasecmp strnicmp
#define strcasecmp stricmp
#define inline __inline
#elseif defined(__WATCOMC__)
#define strncasecmp strnicmp
#define strcasecmp strcmpi
#endif

#if defined(__linux__)
#define O_BINARY 0
#endif

// Basic typedefs.
// NOTE! Somewhere in the code we may still implicitly assume that int = 32 bits, short = 16 bits!
// These should be replaced with the unambiguous types defined below.
#ifdef SDL
#include "SDL3/SDL_stdinc.h"
#else
#include <stdint.h>
typedef int8_t Sint8;
typedef int16_t Sint16;
typedef int32_t Sint32;
#ifdef __WIN32__
typedef __int64 Sint64;
#else
typedef int64_t Sint64;
#endif

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
#endif

// GCC 15 workaround: undef std::byte if it was defined
// This happens when headers like <string> pull in <cstddef> which defines std::byte
#ifdef __GNUC__
namespace std { }
#undef byte
#endif

typedef Uint8 byte;
typedef Uint32 tic_t;
typedef Uint32 angle_t;

struct RGB_t
{
    byte r, g, b;
};

union RGBA_t
{
    Uint32 rgba;
    struct
    {
        byte red;
        byte green;
        byte blue;
        byte alpha;
    };
};

// Predefined with some OS.
#ifndef __WIN32__
#ifndef __MACOS__
#ifndef FREEBSD
#include <values.h>
#else
#include <limits.h>
#endif
#endif
#endif

#ifndef MAXCHAR
#define MAXCHAR ((char)0x7f)
#endif
#ifndef MAXSHORT
#define MAXSHORT ((short)0x7fff)
#endif
#ifndef MAXINT
#define MAXINT ((int)0x7fffffff)
#endif
#ifndef MAXLONG
#define MAXLONG ((long)0x7fffffff)
#endif

#ifndef MINCHAR
#define MINCHAR ((char)0x80)
#endif
#ifndef MINSHORT
#define MINSHORT ((short)0x8000)
#endif
#ifndef MININT
#define MININT ((int)0x80000000)
#endif
#ifndef MINLONG
#define MINLONG ((long)0x80000000)
#endif

#endif
