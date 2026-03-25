// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: EnetNetworkAdapter.cpp 247 2006-04-08 13:16:08Z jussip $
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

#include "net/EnetNetworkAdapter.h"
#include "i_system.h"
#include "doomdef.h"
#include <algorithm>

EnetNetworkAdapter::EnetNetworkAdapter()
    : m_host(nullptr)
    , m_serverPeer(nullptr)
    , m_lastReceivedPeer(nullptr)
    , m_lastConnectedPeerIndex(-1)
    , m_clientCount(0)
    , m_isServer(false)
{
    memset(m_clientPeers, 0, sizeof(m_clientPeers));
    memset(m_receiveBuffer, 0, sizeof(m_receiveBuffer));
}

EnetNetworkAdapter::~EnetNetworkAdapter()
{
    reset();
}

bool EnetNetworkAdapter::initializeServer(uint16_t port, size_t maxClients)
{
    reset();

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    m_host = enet_host_create(&address, static_cast<size_t>(maxClients), 2, 0, 0);
    if (!m_host)
    {
        CONS_Printf("Failed to create ENet server on port %d\n", port);
        return false;
    }

    m_isServer = true;
    m_clientCount = 0;
    memset(m_clientPeers, 0, sizeof(m_clientPeers));

    CONS_Printf("ENet server initialized on port %d\n", port);
    return true;
}

bool EnetNetworkAdapter::initializeClient()
{
    reset();

    m_host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!m_host)
    {
        CONS_Printf("Failed to create ENet client host\n");
        return false;
    }

    m_isServer = false;
    m_serverPeer = nullptr;

    return true;
}

void EnetNetworkAdapter::connect(const char *host, uint16_t port)
{
    if (!m_host || m_isServer)
        return;

    ENetAddress address;
    if (enet_address_set_host(&address, host) < 0)
    {
        CONS_Printf("Failed to resolve host: %s\n", host);
        return;
    }
    address.port = port;

    m_serverPeer = enet_host_connect(m_host, &address, 2, 0);
    if (!m_serverPeer)
    {
        CONS_Printf("Failed to connect to %s:%d\n", host, port);
        return;
    }

    CONS_Printf("Connecting to %s:%d...\n", host, port);
}

void EnetNetworkAdapter::disconnect()
{
    if (m_isServer)
    {
        // Disconnect all clients
        for (size_t i = 0; i < 32; i++)
        {
            if (m_clientPeers[i])
            {
                enet_peer_disconnect(m_clientPeers[i], 0);
                m_clientPeers[i] = nullptr;
            }
        }
        m_clientCount = 0;
    }
    else if (m_serverPeer)
    {
        enet_peer_disconnect(m_serverPeer, 0);
        m_serverPeer = nullptr;
    }
}

void EnetNetworkAdapter::flush()
{
    if (m_host)
    {
        enet_host_flush(m_host);
    }
}

void EnetNetworkAdapter::sendPacket(const void *data, size_t size)
{
    if (!data || size == 0 || !m_host)
        return;

    if (m_isServer)
    {
        broadcast(data, size);
    }
    else if (m_serverPeer)
    {
        ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
        if (packet)
        {
            enet_peer_send(m_serverPeer, 0, packet);
        }
    }
}

void EnetNetworkAdapter::broadcast(const void *data, size_t size)
{
    if (!data || size == 0 || !m_host || !m_isServer)
        return;

    ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    if (packet)
    {
        enet_host_broadcast(m_host, 0, packet);
    }
}

size_t EnetNetworkAdapter::receivePacket(void *buffer, size_t size)
{
    if (!buffer || size == 0 || !m_host)
        return 0;

    ENetEvent event;
    int result = enet_host_service(m_host, &event, 0);

    if (result < 0)
    {
        // Error
        return 0;
    }

    if (result == 0)
    {
        // No event
        return 0;
    }

    switch (event.type)
    {
        case ENET_EVENT_TYPE_CONNECT:
            if (m_isServer)
            {
                // Find empty slot for client
                if (m_clientCount < 32)
                {
                    m_clientPeers[m_clientCount] = event.peer;
                    m_lastConnectedPeerIndex = static_cast<int>(m_clientCount);
                    m_clientCount++;
                    CONS_Printf("Client connected from %x:%u (index %d)\n",
                               event.peer->address.host,
                               event.peer->address.port,
                               m_lastConnectedPeerIndex);
                }
            }
            else
            {
                // Connected to server
                m_serverPeer = event.peer;
                CONS_Printf("Connected to server\n");
            }
            return 0;

        case ENET_EVENT_TYPE_DISCONNECT:
            if (m_isServer)
            {
                // Find and remove client
                for (size_t i = 0; i < m_clientCount; i++)
                {
                    if (m_clientPeers[i] == event.peer)
                    {
                        CONS_Printf("Client disconnected\n");
                        // Shift remaining clients
                        for (size_t j = i; j < m_clientCount - 1; j++)
                        {
                            m_clientPeers[j] = m_clientPeers[j + 1];
                        }
                        m_clientPeers[m_clientCount - 1] = nullptr;
                        m_clientCount--;
                        break;
                    }
                }
            }
            else
            {
                m_serverPeer = nullptr;
                CONS_Printf("Disconnected from server\n");
            }
            return 0;

        case ENET_EVENT_TYPE_RECEIVE:
            if (event.packet && event.packet->data && event.packet->dataLength > 0)
            {
                size_t copySize = std::min(static_cast<size_t>(event.packet->dataLength), size);
                memcpy(buffer, event.packet->data, copySize);

                // Store sender address and peer for routing
                m_lastReceivedFrom = event.peer->address;
                m_lastReceivedPeer = event.peer;

                // Don't destroy packet immediately - the caller might need to process it
                // Note: caller is responsible for destroying or we use a temporary copy
                enet_packet_destroy(event.packet);

                return copySize;
            }
            return 0;

        case ENET_EVENT_TYPE_NONE:
        default:
            return 0;
    }

    return 0;
}

bool EnetNetworkAdapter::isConnected() const
{
    if (m_isServer)
    {
        return m_clientCount > 0;
    }
    else
    {
        return m_serverPeer && m_serverPeer->state == ENET_PEER_STATE_CONNECTED;
    }
}

size_t EnetNetworkAdapter::getConnectedPeersCount() const
{
    if (m_isServer)
    {
        return m_clientCount;
    }
    return m_serverPeer && m_serverPeer->state == ENET_PEER_STATE_CONNECTED ? 1 : 0;
}

ENetPeer *EnetNetworkAdapter::getPeer(size_t index)
{
    if (m_isServer && index < m_clientCount)
    {
        return m_clientPeers[index];
    }
    return nullptr;
}

bool EnetNetworkAdapter::acceptConnection()
{
    // For server: check if there are incoming connection requests
    // This is handled in receivePacket via ENET_EVENT_TYPE_CONNECT
    return false;
}

ENetAddress EnetNetworkAdapter::getLastReceivedFrom() const
{
    return m_lastReceivedFrom;
}

ENetPeer *EnetNetworkAdapter::getLastReceivedPeer() const
{
    return m_lastReceivedPeer;
}

int EnetNetworkAdapter::findPeerByUserData(void *userData) const
{
    for (size_t i = 0; i < m_clientCount; i++)
    {
        if (m_clientPeers[i] && m_clientPeers[i]->data == userData)
            return static_cast<int>(i);
    }
    return -1;
}

void EnetNetworkAdapter::setPeerUserData(ENetPeer *peer, void *userData)
{
    if (peer)
    {
        peer->data = userData;
    }
}

void *EnetNetworkAdapter::getPeerUserData(ENetPeer *peer)
{
    if (peer)
    {
        return peer->data;
    }
    return nullptr;
}

ENetPeerState EnetNetworkAdapter::getPeerState() const
{
    if (m_serverPeer)
    {
        return m_serverPeer->state;
    }
    return ENET_PEER_STATE_DISCONNECTED;
}

void EnetNetworkAdapter::reset()
{
    if (m_host)
    {
        enet_host_destroy(m_host);
        m_host = nullptr;
    }
    m_serverPeer = nullptr;
    m_clientCount = 0;
    memset(m_clientPeers, 0, sizeof(m_clientPeers));
    m_isServer = false;
}
