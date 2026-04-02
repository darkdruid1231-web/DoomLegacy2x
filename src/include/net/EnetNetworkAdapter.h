// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: EnetNetworkAdapter.h 247 2006-04-08 13:16:08Z jussip $
//
// Copyright (C) 2004-2006 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
//
// Description:
//   ENet-based implementation of INetworkAdapter
//
//-----------------------------------------------------------------------------

#ifndef ENETNETWORKADAPTER_H
#define ENETNETWORKADAPTER_H

#include "INetworkAdapter.h"
#include <enet/enet.h>
#include <cstddef>
#include <cstdint>

/**
 * EnetNetworkAdapter - ENet implementation of INetworkAdapter
 *
 * Provides network connectivity using ENet library.
 * Supports both client and server modes.
 */
class EnetNetworkAdapter : public INetworkAdapter
{
  public:
    EnetNetworkAdapter();
    virtual ~EnetNetworkAdapter();

    // INetworkAdapter interface
    virtual void sendPacket(const void *data, size_t size) override;
    virtual size_t receivePacket(void *buffer, size_t size) override;
    virtual bool isConnected() const override;

    // ENet-specific initialization
    bool initializeServer(uint16_t port, size_t maxClients);
    bool initializeClient();

    // Connection management
    void connect(const char *host, uint16_t port);
    void disconnect();
    void flush();

    // Server operations
    void broadcast(const void *data, size_t size);
    size_t getConnectedPeersCount() const;
    ENetPeer *getPeer(size_t index);

    // Check for incoming connection requests (server)
    bool acceptConnection();

    // Get last received from address (for server)
    ENetAddress getLastReceivedFrom() const;

    // Get the peer that sent the last received packet (for routing)
    ENetPeer *getLastReceivedPeer() const;

    // Find client peer by user data (returns index or -1)
    int findPeerByUserData(void *userData) const;

    // Get the index of the most recently connected peer (for LConnection assignment)
    int getLastConnectedPeerIndex() const { return m_lastConnectedPeerIndex; }

    // Set user data on peer
    void setPeerUserData(ENetPeer *peer, void *userData);
    void *getPeerUserData(ENetPeer *peer);

    // Check connection state
    ENetPeerState getPeerState() const;

    // Reset the host
    void reset();

  private:
    ENetHost *m_host;
    ENetPeer *m_serverPeer;  // For clients: connection to server
    ENetPeer *m_clientPeers[32];  // For servers: connections to clients
    ENetPeer *m_lastReceivedPeer;  // Peer that sent the last received packet
    int m_lastConnectedPeerIndex;  // Index of most recently connected client peer
    size_t m_clientCount;
    bool m_isServer;
    uint8_t m_receiveBuffer[1450];  // MAXPACKETLENGTH
    ENetAddress m_lastReceivedFrom;
};

#endif // ENETNETWORKADAPTER_H
