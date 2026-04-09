// Minimal stub definitions for test_console.
//
// test_console.cpp includes command.h and tests only compile-time constants
// (flag enum values, sizeof, struct layout).  The sole runtime link
// dependencies are the three global arrays below; everything else — including
// sizeof(COM), sizeof(command_buffer_t), sizeof(consvar_t), and all CV_*
// enum values — is resolved entirely at compile time.
//
// By providing these three arrays here and linking against util_base (not
// util), the test avoids pulling in command.cpp → z_zone.cpp → m_misc.cpp
// → wad.cpp → dehacked.cpp → ~200 A_* action stubs.

#include "command.h"

CV_PossibleValue_t CV_OnOff[]    = {{0, "Off"}, {1, "On"},         {0, NULL}};
CV_PossibleValue_t CV_YesNo[]    = {{0, "No"},  {1, "Yes"},        {0, NULL}};
CV_PossibleValue_t CV_Unsigned[] = {{0, "MIN"}, {999999999, "MAX"},{0, NULL}};
