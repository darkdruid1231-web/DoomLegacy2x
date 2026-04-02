// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: n_connection.h 342 2006-07-13 19:48:06Z smite-meister $
//
// Copyright (C) 2004-2005 by DooM Legacy Team.
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
//   Network connections using ENet
//
//-----------------------------------------------------------------------------

#ifndef n_connection_h
#define n_connection_h 1

#include "tnl_stub.h"
#include "data/d_ticcmd.h"
#include <list>
#include <vector>

// Forward declarations
class LNetInterface;

// Packet types for direct packet communication
enum PacketType
{
    PKT_CONNECT_REQUEST = 1,
    PKT_CONNECT_ACCEPT,
    PKT_CONNECT_REJECT,
    PKT_DISCONNECT,
    PKT_CHAT,
    PKT_PAUSE,
    PKT_MESSAGE_S2C,
    PKT_NETVAR_S2C,
    PKT_KICK_S2C,
    PKT_OPTIONS_C2S,
    PKT_SUICIDE_C2S,
    PKT_POV_CHANGE_C2S,
    PKT_TICCMD_C2S,
    PKT_GAME_STATE,
    PKT_FULL_STATE,
    PKT_TEST = 0xFF
};

/// \brief Connection between a server and a client using ENet
///
/// Simplified connection class that replaces the TNL GhostConnection.
/// Uses direct packet-based communication via ENet.
/// Maintains GhostConnection inheritance for compatibility with existing code.
class LConnection : public GhostConnection
{
    typedef GhostConnection Parent;

  public:
    std::vector<class PlayerInfo *> player; ///< Serverside: Players on this connection.

    /// Clientside: Local players that wish to join a remote game.
    static std::list<class LocalPlayerInfo *> joining_players;

  public:
    LConnection();
    ~LConnection();

    // Stub methods for ghost replication compatibility
    bool isGhostAvailable(void *p) { return false; }
    void postNetEvent(class NetEvent *e) { (void)e; }

    /// Get the network interface
    LNetInterface *getNetInterface() const { return netInterface; }

    /// Get connection state
    int getConnectionState() const { return connectionState; }

    /// Get network address as string
    const char *getNetAddressString() const;

    /// Get network address
    Address getNetAddress() const;

    /// Check if this is a connection to server (client-side)
    bool isConnectionToServer() const { return !isServer; }

    /// Check if this is the initiator of the connection
    bool isInitiator() const { return initiator; }

    //============================================================
    // Connection management
    //============================================================

    /// Send a packet to this connection
    void sendPacket(const void *data, size_t size);

    /// Handle incoming packet
    void handlePacket(PacketType type, lnet::BitStream *stream);

    //============================================================
    // Connect/Disconnect
    //============================================================

    /// client sends info to server and requests a connection
    void writeConnectRequest(lnet::BitStream *stream);

    /// server decides whether to accept the connection
    bool readConnectRequest(lnet::BitStream *stream, const char **errorString);

    /// server sends info to client
    void writeConnectAccept(lnet::BitStream *stream);

    /// client decides whether to accept the connection
    bool readConnectAccept(lnet::BitStream *stream, const char **errorString);

    /// Called when a pending connection is terminated
    void onConnectTerminated(int reason, const char *reasonStr);

    /// Called when an established connection is terminated
    void onConnectionTerminated(int reason, const char *error);

    /// called on both ends of a connection when the connection is established.
    void onConnectionEstablished();

    /// called when a connection or connection attempt is terminated
    void ConnectionTerminated(bool established);

    //============================================================
    // Simplified RPC-like methods (packet-based)
    //============================================================

    void rpcTest(U8 num);
    void rpcChat(S8 from, S8 to, const char *msg);
    void rpcPause(U8 pnum, bool on);
    void rpcMessage_s2c(S8 pnum, const char *msg, S8 priority, S8 type);
    void rpcSendNetVar_s2c(U16 netid, const char *str);
    void rpcKick_s2c(U8 pnum, const char *str);
    void rpcSendOptions_c2s(U8 pnum, lnet::BitStream *buf);
    void rpcSuicide_c2s(U8 pnum);
    void rpcRequestPOVchange_c2s(S32 pnum);
    void rpcTiccmd_c2s(S32 pnum, const struct ticcmd_t *cmd, uint16_t seq = 0);

  private:
    LNetInterface *netInterface;
    int connectionState;
    bool isServer;
    bool initiator;
    Address remoteAddress;

  public:
    // Peer index for ENet routing (server only)
    int peerIndex;
};

// Termination reasons
enum TerminationReason
{
    ReasonNone = 0,
    ReasonSelfDisconnect,
    ReasonConnectionLost,
    ReasonConnectionRejected
};

#endif
