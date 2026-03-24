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
/// \brief Mock implementation of IRenderBackend for unit testing.

#include "interfaces/i_render.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

class MockRenderBackend : public IRenderBackend {
public:
    MOCK_METHOD(int, getVertexCount, (), (const override));
    MOCK_METHOD(int, getSegCount, (), (const override));
    MOCK_METHOD(int, getSectorCount, (), (const override));
    MOCK_METHOD(int, getSubsectorCount, (), (const override));
    MOCK_METHOD(int, getNodeCount, (), (const override));
    MOCK_METHOD(int, getLineCount, (), (const override));
    MOCK_METHOD(const vertex_t*, getVertexes, (), (const override));
    MOCK_METHOD(const seg_t*, getSegs, (), (const override));
    MOCK_METHOD(const sector_t*, getSectors, (), (const override));
    MOCK_METHOD(const subsector_t*, getSubsectors, (), (const override));
    MOCK_METHOD(const node_t*, getNodes, (), (const override));
    MOCK_METHOD(const line_t*, getLines, (), (const override));
    MOCK_METHOD(const side_t*, getSides, (), (const override));
    MOCK_METHOD(bool, isSubsectorValid, (int index), (const override));
    MOCK_METHOD(bool, isNodeValid, (int index), (const override));
};
