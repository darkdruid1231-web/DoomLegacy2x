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
/// \brief Mock implementation of INetworkProvider for unit testing.

#include "interfaces/i_network_provider.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

class MockNetworkProvider : public INetworkProvider {
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
