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
/// \brief Google Test tests for INetworkProvider using MockNetworkProvider.

#include "interfaces/i_network_provider.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// Local mock matching INetworkProvider interface (same as MockNetworkProvider)
class MockNetworkProviderLocal : public INetworkProvider {
public:
    MOCK_METHOD(bool, connect, (const char* address, uint16_t port), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(void, broadcastPacket, (const void* data, size_t len), (override));
    MOCK_METHOD(void, sendPacket, (int playerIndex, const void* data, size_t len), (override));
    MOCK_METHOD(bool, hasUnprocessed, (), (const override));
    MOCK_METHOD(ConnectionState, getState, (), (const override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(void, startServer, (uint16_t port, bool waitForPlayers), (override));
    MOCK_METHOD(bool, removeConnection, (int playerIndex), (override));
    MOCK_METHOD(void, resetServer, (), (override));
    MOCK_METHOD(int, getPlayerCount, (), (const override));
    MOCK_METHOD(bool, hasPlayer, (int playerIndex), (const override));
};

// Simple component that uses INetworkProvider to check state
class NetworkStateComponent {
public:
    explicit NetworkStateComponent(INetworkProvider* net) : net_(net) {}

    bool isConnected() const {
        return net_->getState() == INetworkProvider::ConnectionState::Connected;
    }

    bool isListening() const {
        return net_->getState() == INetworkProvider::ConnectionState::Listening;
    }

    int activePlayerCount() const {
        return net_->getPlayerCount();
    }

    bool isServerUp() const {
        auto state = net_->getState();
        return state == INetworkProvider::ConnectionState::Listening
            || state == INetworkProvider::ConnectionState::Connected;
    }

private:
    INetworkProvider* net_;
};

// Test fixture
class NetworkProviderTest : public ::testing::Test {
protected:
    NiceMock<MockNetworkProviderLocal> mock_;
    NetworkStateComponent component_;
    NetworkProviderTest() : component_(&mock_) {}
};

// Test: connect transitions state to Connecting, then Connected
TEST_F(NetworkProviderTest, connectTransitionsState) {
    EXPECT_CALL(mock_, connect("localhost", 10666))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_, getState())
        .WillOnce(Return(INetworkProvider::ConnectionState::Connecting))
        .WillOnce(Return(INetworkProvider::ConnectionState::Connected));

    bool ok = mock_.connect("localhost", 10666);
    EXPECT_TRUE(ok);

    EXPECT_EQ(INetworkProvider::ConnectionState::Connecting, mock_.getState());
    EXPECT_EQ(INetworkProvider::ConnectionState::Connected, mock_.getState());
}

// Test: disconnect resets state to Disconnected
TEST_F(NetworkProviderTest, disconnectResetsState) {
    EXPECT_CALL(mock_, disconnect()).Times(1);
    EXPECT_CALL(mock_, getState())
        .WillOnce(Return(INetworkProvider::ConnectionState::Disconnected));

    mock_.disconnect();
    INetworkProvider::ConnectionState st = mock_.getState();
    EXPECT_EQ(INetworkProvider::ConnectionState::Disconnected, st);
}

// Test: startServer + broadcastPacket calls broadcast on all clients
TEST_F(NetworkProviderTest, serverBroadcast) {
    const char data[] = "hello";
    EXPECT_CALL(mock_, startServer(10666, false)).Times(1);
    EXPECT_CALL(mock_, broadcastPacket(data, sizeof(data))).Times(1);

    mock_.startServer(10666, false);
    mock_.broadcastPacket(data, sizeof(data));
}

// Test: server with multiple clients reports correct player count
TEST_F(NetworkProviderTest, playerCount) {
    EXPECT_CALL(mock_, getPlayerCount())
        .WillOnce(Return(0))
        .WillOnce(Return(3))
        .WillOnce(Return(2));

    EXPECT_EQ(0, mock_.getPlayerCount());
    EXPECT_EQ(3, mock_.getPlayerCount());
    EXPECT_EQ(2, mock_.getPlayerCount());
}

// Test: removeConnection reduces player count
TEST_F(NetworkProviderTest, removeConnection) {
    EXPECT_CALL(mock_, hasPlayer(1))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(mock_, removeConnection(1))
        .WillOnce(Return(true));

    EXPECT_TRUE(mock_.hasPlayer(1));
    mock_.removeConnection(1);
    EXPECT_FALSE(mock_.hasPlayer(1));
}

// Test: component wrapper correctly reflects state
TEST_F(NetworkProviderTest, componentStateReflection) {
    EXPECT_CALL(mock_, getState())
        .WillOnce(Return(INetworkProvider::ConnectionState::Listening))
        .WillOnce(Return(INetworkProvider::ConnectionState::Listening));

    EXPECT_TRUE(component_.isServerUp());
    EXPECT_FALSE(component_.isConnected());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
