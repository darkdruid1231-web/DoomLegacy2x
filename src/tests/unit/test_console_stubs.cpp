// Stub implementations to satisfy link-time dependencies of libutil.a
// when building test_console. None of these code paths are exercised
// by the test — they exist only so the linker can build the executable.
//
// Why this file exists
// --------------------
// command.cpp (part of libutil.a) calls CONS_Printf, which is defined in
// console.cpp.  When the linker resolves CONS_Printf it pulls console.cpp.o
// into the link, dragging in vid, materials, hud, G_Key*, font_t, etc.
//
// Providing CONS_Printf here (this .o comes before libutil.a on the link
// line) prevents console.cpp.o from being loaded at all, eliminating the
// bulk of the missing symbols.
//
// The remaining undefined references come from command.cpp.o directly
// (game, LocalPlayers, devparm, I_Error, LNetInterface::SendNetVar) and
// are stubbed out below.

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

// ---------------------------------------------------------------------------
// CONS_Printf — no-op; must be provided before libutil.a in the link order.
// ---------------------------------------------------------------------------
void CONS_Printf(const char *fmt, ...) {}

// ---------------------------------------------------------------------------
// devparm — development mode flag (extern bool devparm in doomdef.h)
// ---------------------------------------------------------------------------
bool devparm = false;

// ---------------------------------------------------------------------------
// I_Error — fatal-error handler; abort so the test binary fails cleanly.
// ---------------------------------------------------------------------------
void I_Error(const char *error, ...)
{
    va_list ap;
    va_start(ap, error);
    vfprintf(stderr, error, ap);
    va_end(ap);
    abort();
}

// ---------------------------------------------------------------------------
// GameInfo game — stub constructor/destructor so the global can be placed
// in the executable.  STL members (maps, vectors, string) are default-
// constructed correctly even with an empty body; POD members are never
// accessed by the test.
// ---------------------------------------------------------------------------
#include "g_game.h"   // wrapper -> game/g_game.h; also declares extern GameInfo game

GameInfo::GameInfo() {}
GameInfo::~GameInfo() {}
GameInfo game;

// ---------------------------------------------------------------------------
// LocalPlayers — stub constructors for the base class and the array.
// NUM_LOCALPLAYERS == 14; objects are never accessed by the test.
// ---------------------------------------------------------------------------
#include "g_player.h" // wrapper -> game/g_player.h

PlayerOptions::PlayerOptions(const std::string &) {}
LocalPlayerInfo::LocalPlayerInfo(const std::string &, int) {}
LocalPlayerInfo LocalPlayers[NUM_LOCALPLAYERS];

// ---------------------------------------------------------------------------
// LNetInterface::SendNetVar — called inside consvar_t::Set(), never reached
// by the test, but must be defined so the linker can close the binary.
// ---------------------------------------------------------------------------
#include "n_interface.h"
void LNetInterface::SendNetVar(U16 /*netid*/, const char * /*str*/) {}
