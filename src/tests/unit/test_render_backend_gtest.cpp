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
/// \brief Google Test tests for IRenderBackend using MockRenderBackend.

#include "interfaces/i_render.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// Mock implementation in the test file
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

//============================================================================
// Example component that depends on IRenderBackend
//============================================================================

class GeometryComponent {
public:
    explicit GeometryComponent(IRenderBackend* backend) : backend_(backend) {}

    /// Get total geometry element count
    int totalElements() const {
        return backend_->getVertexCount() + backend_->getSegCount() +
               backend_->getSectorCount() + backend_->getSubsectorCount() +
               backend_->getNodeCount() + backend_->getLineCount();
    }

    /// Check if a subsector index is valid for rendering
    bool canRenderSubsector(int index) const {
        return backend_->isSubsectorValid(index);
    }

    /// Check if we have any geometry at all
    bool hasGeometry() const {
        return backend_->getVertexCount() > 0 && backend_->getLineCount() > 0;
    }

    /// Check if map has nodes (for BSP traversal)
    bool hasNodes() const {
        return backend_->getNodeCount() > 0;
    }

    /// Get map complexity metric
    int complexity() const {
        return backend_->getLineCount() * 2 + backend_->getSectorCount();
    }

private:
    IRenderBackend* backend_;
};

//============================================================================
// Tests
//============================================================================

TEST(RenderBackend, emptyMap)
{
    NiceMock<MockRenderBackend> mock;

    ON_CALL(mock, getVertexCount()).WillByDefault(Return(0));
    ON_CALL(mock, getSegCount()).WillByDefault(Return(0));
    ON_CALL(mock, getSectorCount()).WillByDefault(Return(0));
    ON_CALL(mock, getSubsectorCount()).WillByDefault(Return(0));
    ON_CALL(mock, getNodeCount()).WillByDefault(Return(0));
    ON_CALL(mock, getLineCount()).WillByDefault(Return(0));

    GeometryComponent geo(&mock);

    EXPECT_EQ(geo.totalElements(), 0);
    EXPECT_FALSE(geo.hasGeometry());
    EXPECT_FALSE(geo.hasNodes());
}

TEST(RenderBackend, typicalMap)
{
    NiceMock<MockRenderBackend> mock;

    ON_CALL(mock, getVertexCount()).WillByDefault(Return(100));
    ON_CALL(mock, getSegCount()).WillByDefault(Return(200));
    ON_CALL(mock, getSectorCount()).WillByDefault(Return(50));
    ON_CALL(mock, getSubsectorCount()).WillByDefault(Return(150));
    ON_CALL(mock, getNodeCount()).WillByDefault(Return(80));
    ON_CALL(mock, getLineCount()).WillByDefault(Return(120));

    GeometryComponent geo(&mock);

    EXPECT_EQ(geo.totalElements(), 700);
    EXPECT_TRUE(geo.hasGeometry());
    EXPECT_TRUE(geo.hasNodes());
    EXPECT_EQ(geo.complexity(), 290);  // 120*2 + 50
}

TEST(RenderBackend, subsectorValidation)
{
    NiceMock<MockRenderBackend> mock;

    ON_CALL(mock, isSubsectorValid(0)).WillByDefault(Return(true));
    ON_CALL(mock, isSubsectorValid(5)).WillByDefault(Return(true));
    ON_CALL(mock, isSubsectorValid(100)).WillByDefault(Return(false));

    GeometryComponent geo(&mock);

    EXPECT_TRUE(geo.canRenderSubsector(0));
    EXPECT_TRUE(geo.canRenderSubsector(5));
    EXPECT_FALSE(geo.canRenderSubsector(100));
}

TEST(RenderBackend, nodeLessMap)
{
    NiceMock<MockRenderBackend> mock;

    ON_CALL(mock, getVertexCount()).WillByDefault(Return(50));
    ON_CALL(mock, getSegCount()).WillByDefault(Return(100));
    ON_CALL(mock, getSectorCount()).WillByDefault(Return(25));
    ON_CALL(mock, getSubsectorCount()).WillByDefault(Return(0));
    ON_CALL(mock, getNodeCount()).WillByDefault(Return(0));
    ON_CALL(mock, getLineCount()).WillByDefault(Return(60));

    GeometryComponent geo(&mock);

    EXPECT_TRUE(geo.hasGeometry());
    EXPECT_FALSE(geo.hasNodes());  // No nodes means no BSP
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
