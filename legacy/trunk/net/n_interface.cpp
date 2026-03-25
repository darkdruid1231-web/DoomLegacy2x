// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: n_interface.cpp 516 2007-12-22 15:57:58Z jussip $
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
//   Legacy Network Interface using ENet
//
//-----------------------------------------------------------------------------

#include "command.h"
#include "cvars.h"
#include "doomdef.h"

#include "n_connection.h"
#include "n_interface.h"

#include "g_game.h"
#include "g_type.h"

#include "i_system.h"

#include "m_misc.h"
#include "vfile.h"
#include "w_wad.h"

//===================================================================
//    Server lists
//===================================================================

serverinfo_t::serverinfo_t(const Address &a)
{
    addr = a;
    nextquery = 0;
    version = 0;
    players = maxplayers = 0;
    gt_version = 0;
}

/// reads server information from a packet
void serverinfo_t::Read(BitStream &s)
{
    char temp[256];

    s.read(&version);
    s.readString(temp);
    versionstring = temp;

    s.readString(temp);
    name = temp;

    s.read(&players);
    s.read(&maxplayers);

    s.readString(temp);
    gt_name = temp;
    s.read(&gt_version);
}

/// writes the current server information from to a packet
void serverinfo_t::Write(BitStream &s)
{
    s.write(game.demoversion);
    s.writeString(LEGACY_VERSIONSTRING);
    s.writeString(cv_servername.str);
    s.write(static_cast<unsigned int>(game.Players.size()));
    s.write(game.maxplayers);
    s.writeString(game.gtype->gt_name.c_str());
    s.write(game.gtype->gt_version);
    // TODO more basic info? current mapname? server load?
}

serverinfo_t *LNetInterface::SL_FindServer(const Address &a)
{
    int n = serverlist.size();
    for (int i = 0; i < n; i++)
        if (serverlist[i]->addr == a)
            return serverlist[i];

    return NULL;
}

serverinfo_t *LNetInterface::SL_AddServer(const Address &a)
{
    if (serverlist.size() >= 50)
        return NULL; // no more room

    serverinfo_t *s = new serverinfo_t(a);
    serverlist.push_back(s);
    return s;
}

void LNetInterface::SL_Clear()
{
    int n = serverlist.size();
    for (int i = 0; i < n; i++)
        delete serverlist[i];

    serverlist.clear();
}

void LNetInterface::SL_Update()
{
    int n = serverlist.size();
    for (int i = 0; i < n; i++)
        if (serverlist[i]->nextquery <= nowtime)
            SendQuery(serverlist[i]);
}

//===================================================================
//     Network Interface
//===================================================================

static const char *ConnectionState[] = {
    "Not connected",
    "Awaiting challenge response",
    "Sending punch packets",
    "Computing puzzle solution",
    "Awaiting connect response",
    "Connect timeout",
    "Connection rejected",
    "Connected.",
    "Disconnected",
    "Connection timed out"
};

LNetInterface::LNetInterface(const Address &bind)
    : adapter(nullptr)
    , master_con(nullptr)
    , server_con(nullptr)
    , netstate(SV_Unconnected)
    , nowtime(0)
    , nextpingtime(0)
    , autoconnect(false)
    , nodownload(false)
{
    (void)bind; // unused for now

    // Create ENet adapter for client mode initially
    adapter = new EnetNetworkAdapter();
}

LNetInterface::~LNetInterface()
{
    if (adapter)
    {
        adapter->reset();
        delete adapter;
        adapter = nullptr;
    }
}

void LNetInterface::CL_StartPinging(bool connectany)
{
    autoconnect = connectany;
    nextpingtime = I_GetTime();
    pingnonce.getRandom();
    SL_Clear();

    // Initialize as client if not already
    if (!adapter->initializeClient())
    {
        CONS_Printf("Failed to initialize ENet client\n");
        return;
    }

    netstate = CL_PingingServers;
}

/// send out a server ping
void LNetInterface::SendPing(const Address &a, const Nonce &cn)
{
    if (devparm)
        CONS_Printf("Sending out server ping to %s...\n", a.toString());

    uint8_t buffer[256];
    lnet::BitStream bs(buffer, sizeof(buffer));
    bs.write(U8(PT_ServerPing));
    cn.write(&bs);
    bs.write(LEGACY_VERSION);
    bs.writeString(LEGACY_VERSIONSTRING);
    bs.write(nowtime);

    // TODO: Send via ENet broadcast
    // For now, use adapter's sendPacket which will broadcast on server
    // This is a simplification - LAN discovery needs proper UDP broadcast

    nextpingtime = nowtime + PingDelay;
}

// send out server query
void LNetInterface::SendQuery(serverinfo_t *s)
{
    if (devparm)
        CONS_Printf("Querying server %s...\n", s->addr.toString());

    uint8_t buffer[256];
    lnet::BitStream bs(buffer, sizeof(buffer));
    bs.write(U8(PT_ServerQuery));
    s->cn.write(&bs);
    bs.write(s->token);

    // TODO: Send via ENet to specific address
    // adapter->sendPacketTo(server, buffer, bs.getNumBytes());

    s->nextquery = nowtime + QueryDelay;
}

/// Handles received info packets
void LNetInterface::handleInfoPacket(const Address &address, U8 packetType, BitStream *stream)
{
    // first 8 bits denote the packet type
    switch (packetType)
    {
        case PT_ServerPing:
            if (game.server && netstate != SV_Unconnected)
            {
                if (devparm)
                    CONS_Printf("received ping from %s\n", address.toString());

                Nonce cn;
                cn.read(stream);

                int version;
                stream->read(&version);
                if (version != LEGACY_VERSION)
                {
                    CONS_Printf("Wrong version (%d.%d)\n", version / 100, version % 100);
                    break;
                }
                char temp[256];
                stream->readString(temp);
                if (devparm)
                    CONS_Printf(" versionstring '%s'\n", temp);

                unsigned time;
                stream->read(&time);

                U32 token = computeClientIdentityToken(address, cn);

                uint8_t outbuf[256];
                lnet::BitStream outbs(outbuf, sizeof(outbuf));
                outbs.write(U8(PT_PingResponse));
                cn.write(&outbs);
                outbs.write(token);
                outbs.write(time);

                // TODO: Send response via ENet
            }
            break;

        case PT_PingResponse:
            if (netstate == CL_PingingServers)
            {
                if (devparm)
                    CONS_Printf("received ping response from %s\n", address.toString());

                Nonce cn;
                cn.read(stream);

                if (cn != pingnonce)
                    return; // wrong nonce

                if (SL_FindServer(address))
                    return; // already known

                serverinfo_t *s = SL_AddServer(address);
                if (!s)
                    return;

                s->cn = cn;
                stream->read(&s->token);

                unsigned time;
                stream->read(&time);
                s->ping = nowtime - time;
                if (devparm)
                    CONS_Printf("ping: %d ms\n", s->ping);

                SendQuery(s);
            }
            break;

        case PT_ServerQuery:
            if (game.server && netstate != SV_Unconnected)
            {
                if (devparm)
                    CONS_Printf("Got server query from %s\n", address.toString());

                Nonce cn;
                cn.read(stream);

                U32 token;
                stream->read(&token);

                if (token == computeClientIdentityToken(address, cn))
                {
                    uint8_t outbuf[512];
                    lnet::BitStream outbs(outbuf, sizeof(outbuf));
                    outbs.write(U8(PT_QueryResponse));
                    cn.write(&outbs);

                    serverinfo_t::Write(outbs);

                    // TODO: Send via ENet
                }
            }
            break;

        case PT_QueryResponse:
            if (netstate == CL_PingingServers)
            {
                if (devparm)
                    CONS_Printf("Got query response from %s\n", address.toString());

                serverinfo_t *s = SL_FindServer(address);
                if (!s)
                    return;

                Nonce cn;
                cn.read(stream);
                if (cn != s->cn)
                    return;

                s->Read(*stream);

                if (autoconnect)
                    CL_Connect(address);
            }
            break;

        default:
            if (devparm)
                CONS_Printf("unknown packet %d from %s", packetType, address.toString());
            break;
    }
}

// client making a connection
void LNetInterface::CL_Connect(const Address &a)
{
    game.server = false;
    game.netgame = true;
    game.multiplayer = true;

    // Close client adapter and create connection to server
    if (adapter)
    {
        adapter->reset();
        delete adapter;
    }

    adapter = new EnetNetworkAdapter();
    if (!adapter->initializeClient())
    {
        CONS_Printf("Failed to initialize ENet client\n");
        delete adapter;
        adapter = nullptr;
        return;
    }

    // Convert Address to host string and port
    // For now, simplified - would need Address::toString() implementation
    adapter->connect("127.0.0.1", 5029); // TODO: proper address conversion

    server_con = new LConnection();
    netstate = CL_Connecting;
}

void LNetInterface::CL_Reset()
{
    if (server_con)
    {
        // TODO: proper disconnect
        delete server_con;
        server_con = nullptr;
    }

    if (adapter)
    {
        adapter->disconnect();
    }

    netstate = SV_Unconnected;
}

void LNetInterface::SV_Open(bool wait)
{
    // Close client adapter and create server
    if (adapter)
    {
        adapter->reset();
        delete adapter;
    }

    adapter = new EnetNetworkAdapter();
    uint16_t port = 5029; // Default port
    if (!adapter->initializeServer(port, 16))
    {
        CONS_Printf("Failed to initialize ENet server\n");
        delete adapter;
        adapter = nullptr;
        return;
    }

    if (wait)
        netstate = SV_WaitingClients;
    else
        netstate = SV_Running;
}

bool LNetInterface::SV_RemoveConnection(LConnection *c)
{
    vector<LConnection *>::iterator t = client_con.begin();
    for (; t != client_con.end(); t++)
        if (*t == c)
        {
            client_con.erase(t);
            return true;
        }

    return false;
}

void LNetInterface::SV_Reset()
{
    vector<LConnection *>::iterator t = client_con.begin();
    for (; t != client_con.end(); t++)
    {
        // TODO: proper disconnect
        delete *t;
    }
    client_con.clear();

    if (master_con)
    {
        delete master_con;
        master_con = nullptr;
    }

    if (adapter)
    {
        adapter->reset();
    }

    CL_Reset();
}

void LNetInterface::Update()
{
    nowtime = I_GetTime();

    switch (netstate)
    {
        case CL_PingingServers:
            if (nextpingtime <= nowtime)
                SendPing(ping_address, pingnonce);

            SL_Update();
            break;

        case CL_Connecting:
            // Check connection state
            if (server_con && adapter && adapter->isConnected())
            {
                netstate = CL_Connected;
                CONS_Printf("Connected to server!\n");
            }
            break;

        default:
            break;
    }

    // Process incoming packets if adapter exists
    if (adapter)
    {
        uint8_t buffer[1450];
        size_t len = adapter->receivePacket(buffer, sizeof(buffer));
        if (len > 0 && buffer[0] != 0)
        {
            // Determine packet type (first byte)
            uint8_t packetType = buffer[0];

            // Route packet to appropriate connection
            if (netstate == CL_Connected && server_con)
            {
                // Client: all game packets go to server connection
                if (packetType >= PKT_CHAT)
                {
                    lnet::BitStream bs(buffer, len);
                    bs.readUInt8(); // consume packet type
                    server_con->handlePacket(static_cast<PacketType>(packetType), &bs);
                }
            }
            else if (netstate == SV_Running || netstate == SV_WaitingClients)
            {
                // Server: route to specific client connection
                // Find client by peer index stored in LConnection
                ENetPeer *peer = adapter->getLastReceivedPeer();
                if (peer)
                {
                    // Find which client has the matching peer index
                    // The peer index corresponds to the order clients connected
                    for (size_t i = 0; i < client_con.size(); i++)
                    {
                        if (client_con[i]->peerIndex >= 0)
                        {
                            ENetPeer *clientPeer = adapter->getPeer(client_con[i]->peerIndex);
                            if (clientPeer == peer)
                            {
                                lnet::BitStream bs(buffer, len);
                                bs.readUInt8(); // consume packet type
                                client_con[i]->handlePacket(static_cast<PacketType>(packetType), &bs);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void LNetInterface::broadcastPacket(const void *data, size_t size)
{
    if (adapter)
        adapter->broadcast(data, size);
}

void LNetInterface::sendPacketTo(LConnection *conn, const void *data, size_t size)
{
    if (adapter && conn)
    {
        // Find the peer's index and send directly
        for (size_t i = 0; i < adapter->getConnectedPeersCount(); i++)
        {
            ENetPeer *peer = adapter->getPeer(i);
            if (peer)
            {
                ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(peer, 0, packet);
            }
        }
    }
}

// RPC stubs - simplified packet-based implementations
void LNetInterface::SendChat(int from, int to, const char *msg)
{
    uint8_t buf[512];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(0x10)); // PKT_CHAT
    bs.write(U8(from));
    bs.write(U8(to));
    bs.writeString(msg);

    if (game.server)
        broadcastPacket(buf, bs.getNumBytes());
    else if (server_con)
        server_con->sendPacket(buf, bs.getNumBytes());
}

void LNetInterface::Pause(int pnum, bool on)
{
    uint8_t buf[16];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(0x11)); // PKT_PAUSE
    bs.write(U8(pnum));
    bs.writeFlag(on);

    if (game.server)
        broadcastPacket(buf, bs.getNumBytes());
    else if (server_con)
        server_con->sendPacket(buf, bs.getNumBytes());
}

void LNetInterface::SendNetVar(U16 netid, const char *str)
{
    uint8_t buf[512];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(0x12)); // PKT_NETVAR
    bs.write(netid);
    bs.writeString(str);

    if (game.server)
        broadcastPacket(buf, bs.getNumBytes());
}

void LNetInterface::SendPlayerOptions(int pnum, class LocalPlayerInfo &p)
{
    // TODO
}

void LNetInterface::RequestSuicide(int pnum)
{
    uint8_t buf[8];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(0x13)); // PKT_SUICIDE
    bs.write(U8(pnum));

    if (server_con)
        server_con->sendPacket(buf, bs.getNumBytes());
}

void LNetInterface::Kick(class PlayerInfo *p)
{
    // TODO
}

void LNetInterface::OnReceiveTiccmd(LConnection *conn, S32 pnum, const ticcmd_t *cmd, uint16_t seq)
{
    // Store ticcmd in the correct player's pending slot
    if (conn && conn->isServer)
    {
        // Find the player with matching number in this connection
        for (size_t i = 0; i < conn->player.size(); i++)
        {
            PlayerInfo *p = conn->player[i];
            if (p && p->number == pnum)
            {
                // Check for lost or duplicate packets using sequence numbers
                uint16_t expected = p->last_seq + 1;
                if (seq != 0 && p->last_seq != 0)
                {
                    if (seq < expected)
                    {
                        // Duplicate or out-of-order packet, ignore
                        CONS_Printf("Duplicate ticcmd: seq=%d expected=%d\n", seq, expected);
                        return;
                    }
                    else if (seq > expected)
                    {
                        // Lost packet(s) - just log for now, process anyway
                        CONS_Printf("Lost ticcmd(s): seq=%d expected=%d\n", seq, expected);
                    }
                }
                // Update last received sequence
                p->last_seq = seq;

                // Store ticcmd for this player - server will apply it during Ticker
                p->pendingCmd = *cmd;
                p->hasPendingCmd = true;
                return;
            }
        }
    }
}

void LNetInterface::BroadcastGameState()
{
    if (netstate != SV_Running)
        return;

    uint8_t buf[2048];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_GAME_STATE));
    bs.write(static_cast<uint32_t>(game.tic));

    // Number of players
    uint8_t numPlayers = static_cast<uint8_t>(game.Players.size());
    bs.write(numPlayers);

    // Write each player's state
    for (int i = 0; i < numPlayers; i++)
    {
        PlayerInfo *p = game.Players[i];
        bs.write(static_cast<uint8_t>(p->number));

        if (p->pawn)
        {
            bs.write(static_cast<uint8_t>(1)); // has mobj
            bs.write(p->pawn->x);
            bs.write(p->pawn->y);
            bs.write(p->pawn->z);
            bs.write(p->pawn->angle);
            bs.write(static_cast<uint16_t>(p->pawn->state ? p->pawn->state->index : 0));
        }
        else
        {
            bs.write(static_cast<uint8_t>(0)); // no mobj
        }
    }

    broadcastPacket(buf, bs.getNumBytes());
}

void FileCache::WriteNetInfo(BitStream &s)
{
    S32 n = vfiles.size();
    s.write(n);

    for (int i = 0; i < n; i++)
    {
        S32 size;
        byte md5[16];

        s.write(vfiles[i]->GetNetworkInfo(&size, md5));
        s.writeString(FIL_StripPath(vfiles[i]->filename.c_str()));
        s.write(size);
        s.write(16, md5);
    }
}
