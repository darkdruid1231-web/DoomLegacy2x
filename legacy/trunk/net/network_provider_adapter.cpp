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
/// \brief Adapter implementing INetworkProvider by delegating to LNetInterface.

#include "interfaces/i_network_provider.h"
#include "n_interface.h"
#include "game/g_game.h"
#include <cstring>
#include <cstdio>

/// \brief Adapter that implements INetworkProvider by delegating to the global LNetInterface.
/// \details This allows production code to use INetworkProvider pointers while
///         still accessing the real LNetInterface. In tests, MockNetworkProvider
///         can be injected instead.
class NetworkProviderAdapter : public INetworkProvider {
public:
    bool connect(const char* address, uint16_t port) override {
        if (!game.net)
            return false;
        char buf[256];
        // Address takes a "host:port" or just "host" string
        if (port != 0)
            snprintf(buf, sizeof(buf), "%s:%u", address, port);
        else
            snprintf(buf, sizeof(buf), "%s", address);
        Address addr(buf);
        game.net->CL_Connect(addr);
        return true;
    }

    void disconnect() override {
        if (!game.net)
            return;
        if (game.server)
            game.net->SV_Reset();
        else
            game.net->CL_Reset();
    }

    void broadcastPacket(const void* data, size_t len) override {
        if (!game.net)
            return;
        game.net->broadcastPacket(data, len);
    }

    void sendPacket(int playerIndex, const void* data, size_t len) override {
        if (!game.net)
            return;
        if (game.server) {
            // Server: playerIndex indexes into client_con vector
            if (playerIndex >= 0 && playerIndex < (int)game.net->client_con.size()) {
                LConnection* conn = game.net->client_con[playerIndex];
                if (conn)
                    game.net->sendPacketTo(conn, data, len);
            }
        } else {
            // Client: playerIndex 0 means server
            if (playerIndex == 0 && game.net->server_con)
                game.net->sendPacketTo(game.net->server_con, data, len);
        }
    }

    bool hasUnprocessed() const override {
        if (!game.net || !game.net->adapter)
            return false;
        // receivePacket with 0 timeout just polls - returns >0 if events processed
        uint8_t buf[1];
        size_t r = game.net->adapter->receivePacket(buf, 0);
        return r > 0;
    }

    ConnectionState getState() const override {
        if (!game.net)
            return ConnectionState::Disconnected;
        switch (game.net->netstate) {
            case LNetInterface::SV_Unconnected:
                return ConnectionState::Disconnected;
            case LNetInterface::CL_Connecting:
                return ConnectionState::Connecting;
            case LNetInterface::CL_Connected:
            case LNetInterface::SV_Running:
            case LNetInterface::SV_WaitingClients:
                return ConnectionState::Connected;
        }
        return ConnectionState::Disconnected;
    }

    void update() override {
        if (!game.net)
            return;
        game.net->Update();
    }

    void startServer(uint16_t port, bool waitForPlayers) override {
        if (!game.net)
            return;
        // SV_Open takes a bool for waitforplayers
        game.net->SV_Open(waitForPlayers);
        (void)port;  // port is set at construction time in LNetInterface
    }

    bool removeConnection(int playerIndex) override {
        if (!game.net || !game.server)
            return false;
        if (playerIndex >= 0 && playerIndex < (int)game.net->client_con.size()) {
            LConnection* conn = game.net->client_con[playerIndex];
            if (conn) {
                bool result = game.net->SV_RemoveConnection(conn);
                return result;
            }
        }
        return false;
    }

    void resetServer() override {
        if (!game.net)
            return;
        game.net->SV_Reset();
    }

    int getPlayerCount() const override {
        if (!game.net)
            return 0;
        int count = 0;
        for (LConnection* conn : game.net->client_con) {
            if (conn)
                ++count;
        }
        return count;
    }

    bool hasPlayer(int playerIndex) const override {
        if (!game.net)
            return false;
        return playerIndex >= 0
            && playerIndex < (int)game.net->client_con.size()
            && game.net->client_con[playerIndex] != nullptr;
    }
};

// Global adapter instance
static NetworkProviderAdapter s_networkProviderAdapter;

/// \brief Get the global INetworkProvider instance.
/// \details Returns an adapter pointing to the global LNetInterface.
///         Production code should use this instead of directly accessing game.net.
INetworkProvider* GetGlobalNetworkProvider() {
    return &s_networkProviderAdapter;
}
