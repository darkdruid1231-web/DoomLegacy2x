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
/// \brief Network provider interface for dependency injection in tests.
///
/// This interface abstracts the LNetInterface-based networking (client/server,
/// packet sending, connection management) to enable mock-based unit testing.
///
/// Usage in production:
///   INetworkProvider* net = GetGlobalNetworkProvider(); // returns adapter
///
/// Usage in tests:
///   MockNetworkProvider mock;
///   NetworkRunner runner(&mock);  // injects mock

#ifndef INCLUDED_INTERFACES_I_NETWORK_PROVIDER_H
#define INCLUDED_INTERFACES_I_NETWORK_PROVIDER_H

#include <cstddef>
#include <cstdint>

class INetworkProvider {
public:
    virtual ~INetworkProvider() = default;

    /// Connection state of the local endpoint.
    enum class ConnectionState {
        Disconnected,  ///< Not connected to any server, server not running
        Connecting,   ///< Client attempting to connect to a server
        Connected,    ///< Client connected to server, or server running with clients
        Listening      ///< Server open and waiting for clients
    };

    // --- Connection Management ---

    /// Connect as a client to a server.
    /// \param address Server hostname or IP string
    /// \param port Server port number
    /// \return true if connection attempt was initiated
    virtual bool connect(const char* address, uint16_t port) = 0;

    /// Disconnect from server (client) or close server (server).
    virtual void disconnect() = 0;

    // --- Packet Sending ---

    /// Broadcast a packet to all connected clients (server only).
    /// No-op on client.
    virtual void broadcastPacket(const void* data, size_t len) = 0;

    /// Send a packet to a specific client by player index.
    /// \param playerIndex 0-based index into client connection table
    /// \param data Raw packet bytes
    /// \param len Byte length of packet data
    virtual void sendPacket(int playerIndex, const void* data, size_t len) = 0;

    // --- State & Polling ---

    /// Returns true if there are unprocessed incoming network events.
    virtual bool hasUnprocessed() const = 0;

    /// Returns the current connection state.
    virtual ConnectionState getState() const = 0;

    /// Pump the network: receive packets, process events, retransmit.
    /// Called once per game tick.
    virtual void update() = 0;

    // --- Server Operations ---

    /// Open a server on the given port.
    /// \param port UDP port to listen on
    /// \param waitForPlayers If true, server waits for players before starting
    virtual void startServer(uint16_t port, bool waitForPlayers) = 0;

    /// Remove a client connection by player index.
    /// \param playerIndex 0-based index into client connection table
    /// \return true if a client was removed
    virtual bool removeConnection(int playerIndex) = 0;

    /// Reset server: close all client connections and stop listening.
    virtual void resetServer() = 0;

    // --- Player/Connection Queries ---

    /// Returns the number of active client connections.
    virtual int getPlayerCount() const = 0;

    /// Returns true if a player with the given index exists.
    virtual bool hasPlayer(int playerIndex) const = 0;
};

#endif // INCLUDED_INTERFACES_I_NETWORK_PROVIDER_H
