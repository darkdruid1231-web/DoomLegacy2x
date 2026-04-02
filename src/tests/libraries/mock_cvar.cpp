//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Mock implementation of ICVarProvider for unit testing.

#include "interfaces/i_cvar.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

class MockCVarProvider : public ICVarProvider {
public:
    MOCK_METHOD(int, getInt, (const char* name), (const override));
    MOCK_METHOD(const char*, getString, (const char* name), (const override));
    MOCK_METHOD(bool, setInt, (const char* name, int value), (override));
    MOCK_METHOD(bool, exists, (const char* name), (const override));
};
