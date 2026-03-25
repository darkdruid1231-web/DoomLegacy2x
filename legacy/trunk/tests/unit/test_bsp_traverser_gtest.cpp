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
/// \brief Google Test tests for BSPTraverserComponent using MockRenderBackend.

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

// Local mock matching IRenderBackend interface
class MockRenderBackendLocal : public IRenderBackend {
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

// BSP traversal component that validates node/subsector geometry
class BSPTraverserComponent {
public:
    explicit BSPTraverserComponent(IRenderBackend* render) : render_(render) {}

    bool canTraverse() const {
        return render_->getNodeCount() > 0 && render_->getSubsectorCount() > 0;
    }

    int getExpectedSubsectorCount() const {
        int nodes = render_->getNodeCount();
        return nodes > 0 ? nodes * 2 : render_->getSubsectorCount();
    }

    bool validateBSP() const {
        if (render_->getNodeCount() == 0) {
            return render_->getSubsectorCount() == 1;  // Leaf-only BSP
        }
        return render_->isNodeValid(render_->getNodeCount() - 1);
    }

    int totalElements() const {
        return render_->getVertexCount() + render_->getSegCount() +
               render_->getSectorCount() + render_->getSubsectorCount() +
               render_->getNodeCount() + render_->getLineCount();
    }

private:
    IRenderBackend* render_;
};

// Tests

TEST(BSPTraverser, canTraverseWhenNodesExist)
{
    NiceMock<MockRenderBackendLocal> mock;

    ON_CALL(mock, getNodeCount()).WillByDefault(Return(100));
    ON_CALL(mock, getSubsectorCount()).WillByDefault(Return(150));

    BSPTraverserComponent traverser(&mock);
    EXPECT_TRUE(traverser.canTraverse());
}

TEST(BSPTraverser, cannotTraverseWithoutNodes)
{
    NiceMock<MockRenderBackendLocal> mock;

    ON_CALL(mock, getNodeCount()).WillByDefault(Return(0));
    ON_CALL(mock, getSubsectorCount()).WillByDefault(Return(1));

    BSPTraverserComponent traverser(&mock);
    EXPECT_FALSE(traverser.canTraverse());
}

TEST(BSPTraverser, validatesBSPIndexBounds)
{
    NiceMock<MockRenderBackendLocal> mock;

    ON_CALL(mock, getNodeCount()).WillByDefault(Return(50));
    ON_CALL(mock, isNodeValid(49)).WillByDefault(Return(true));

    BSPTraverserComponent traverser(&mock);
    EXPECT_TRUE(traverser.validateBSP());
}

TEST(BSPTraverser, rejectsInvalidNodeIndex)
{
    NiceMock<MockRenderBackendLocal> mock;

    ON_CALL(mock, getNodeCount()).WillByDefault(Return(50));
    ON_CALL(mock, isNodeValid(49)).WillByDefault(Return(false));

    BSPTraverserComponent traverser(&mock);
    EXPECT_FALSE(traverser.validateBSP());
}

TEST(BSPTraverser, countsTotalElements)
{
    NiceMock<MockRenderBackendLocal> mock;

    ON_CALL(mock, getVertexCount()).WillByDefault(Return(100));
    ON_CALL(mock, getSegCount()).WillByDefault(Return(200));
    ON_CALL(mock, getSectorCount()).WillByDefault(Return(50));
    ON_CALL(mock, getSubsectorCount()).WillByDefault(Return(150));
    ON_CALL(mock, getNodeCount()).WillByDefault(Return(80));
    ON_CALL(mock, getLineCount()).WillByDefault(Return(120));

    BSPTraverserComponent traverser(&mock);
    EXPECT_EQ(traverser.totalElements(), 700);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
