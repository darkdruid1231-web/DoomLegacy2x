// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: n_connection.cpp 500 2007-11-09 00:37:13Z smite-meister $
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

#include "command.h"
#include "cvars.h"
#include "doomdef.h"

#include "n_connection.h"
#include "n_interface.h"

#include "g_game.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_type.h"

#include "w_wad.h"

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

extern unsigned num_bots;

list<class LocalPlayerInfo *> LConnection::joining_players;

LConnection::LConnection()
    : netInterface(nullptr)
    , connectionState(StateNotConnected)
    , isServer(false)
    , initiator(false)
    , peerIndex(-1)
{
}

LConnection::~LConnection()
{
}

const char *LConnection::getNetAddressString() const
{
    static char buf[64];
    // Simple implementation - would need proper ENet address conversion
    sprintf(buf, "%08x:%d", remoteAddress.hash(), 0);
    return buf;
}

Address LConnection::getNetAddress() const
{
    return remoteAddress;
}

void LConnection::sendPacket(const void *data, size_t size)
{
    if (netInterface && netInterface->adapter)
    {
        // Send to specific connection
        netInterface->sendPacketTo(this, data, size);
    }
}

// client
void LConnection::writeConnectRequest(lnet::BitStream *stream)
{
    stream->write(LEGACY_VERSION);
    stream->writeString(LEGACY_VERSIONSTRING);

    joining_players.clear();

    // send local player data
    unsigned n = 1 + cv_splitscreen.value; // number of local human players
    stream->write(n + num_bots);

    LocalPlayerInfo *p;
    // first humans
    for (unsigned i = 0; i < n; i++)
    {
        p = &LocalPlayers[i];
        p->Write(stream);
        joining_players.push_back(p);
    }

    // then bots
    for (unsigned i = 0; i < num_bots; i++)
    {
        p = &LocalPlayers[NUM_LOCALHUMANS + i];
        p->Write(stream);
        joining_players.push_back(p);
    }
}

// server
bool LConnection::readConnectRequest(lnet::BitStream *stream, const char **errorString)
{
    static char temp[256];
    *errorString = temp;

    int version;
    stream->read(&version);
    stream->readString(temp);

    if (version != LEGACY_VERSION || strcmp(temp, LEGACY_VERSIONSTRING))
    {
        sprintf(temp,
                "Different Legacy versions cannot play a net game! (Server version %d.%d%s)",
                LEGACY_VERSION / 100,
                LEGACY_VERSION % 100,
                LEGACY_VERSIONSTRING);
        return false;
    }

    if (!cv_allownewplayers.value)
    {
        sprintf(temp, "The server is not accepting new players at the moment.");
        return false;
    }

    // how many players want to get in?
    unsigned n;
    stream->read(&n);

    if (netInterface->netstate == LNetInterface::SV_Running)
    {
        if (game.Players.size() >= unsigned(cv_maxplayers.value))
        {
            sprintf(temp, "Maximum number of players reached (%d).", cv_maxplayers.value);
            return false;
        }

        n = min(n, cv_maxplayers.value - game.Players.size());

        // read joining players' preferences
        for (unsigned i = 0; i < n; i++)
        {
            PlayerInfo *p = new PlayerInfo();
            p->options.Read(stream);
            p->connection = this;
            p->client_hash = remoteAddress.hash();

            p->name = p->options.name;

            player.push_back(p);
            if (!game.AddPlayer(p))
                I_Error("shouldn't happen! rotten!\n");
        }
    }
    else
    {
        // Waiting for players
    }

    return true;
}

// server
void LConnection::writeConnectAccept(lnet::BitStream *stream)
{
    unsigned n = player.size();
    stream->write(n);

    for (unsigned i = 0; i < n; i++)
        stream->write(player[i]->number);

    serverinfo_t::Write(*stream);
    game.gtype->WriteServerInfo(*stream);
}

// client
bool LConnection::readConnectAccept(lnet::BitStream *stream, const char **errorString)
{
    unsigned n;
    stream->read(&n);
    if (n == 0 || n > joining_players.size())
        return false;

    CONS_Printf("Server accepts %d players.\n", n);
    joining_players.resize(n);

    list<class LocalPlayerInfo *>::iterator t = joining_players.begin();
    for (; t != joining_players.end(); t++)
        stream->read(&(*t)->pnumber);

    // read server properties
    serverinfo_t *s = new serverinfo_t(remoteAddress);
    s->Read(*stream);

    // TODO check if we have the correct gametype DLL!
    game.gtype->ReadServerInfo(*stream);

    return true;
}

void LConnection::onConnectTerminated(int reason, const char *reasonStr)
{
    CONS_Printf("Connect terminated (%d), %s\n", reason, reasonStr);
    ConnectionTerminated(false);
}

void LConnection::onConnectionEstablished()
{
    if (isInitiator())
    {
        // client side
        netInterface->server_con = this;
        netInterface->netstate = LNetInterface::CL_Connected;
        CONS_Printf("Connected to server at %s.\n", getNetAddressString());

        rpcTest(7467);

        game.CL_SpawnClient();
        game.CL_StartGame();
    }
    else
    {
        // server side - this is a new client connection
        peerIndex = netInterface->adapter->getLastConnectedPeerIndex();
        netInterface->client_con.push_back(this);

        int k = player.size();
        for (int i = 0; i < k; i++)
        {
            CONS_Printf("%s entered the game (player %d)\n", player[i]->name.c_str(), player[i]->number);
        }
    }
}

void LConnection::onConnectionTerminated(int reason, const char *error)
{
    CONS_Printf("%s - connection to %s: %s\n",
                getNetAddressString(),
                isConnectionToServer() ? "server" : "client",
                error);
    ConnectionTerminated(true);
}

void LConnection::ConnectionTerminated(bool established)
{
    if (isConnectionToServer())
    {
        netInterface->CL_Reset();
    }
    else
    {
        if (established)
        {
            int n = player.size();
            for (int i = 0; i < n; i++)
            {
                CONS_Printf("Player (%d) dropped.\n", player[i]->number);
                player[i]->playerstate = PST_REMOVE;
            }
            netInterface->SV_RemoveConnection(this);
        }
        else
        {
            CONS_Printf("Unsuccesful connect attempt\n");
        }
    }
}

//========================================================
//            Remote Procedure Calls (Packet-based)
//
// Functions that are called at one end of the
// connection and executed at the other.
//========================================================

void LConnection::rpcTest(U8 num)
{
    uint8_t buf[16];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_TEST));
    bs.write(num);
    sendPacket(buf, bs.getNumBytes());
}

void LConnection::rpcChat(S8 from, S8 to, const char *msg)
{
    uint8_t buf[512];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_CHAT));
    bs.write(from);
    bs.write(to);
    bs.writeString(msg);
    sendPacket(buf, bs.getNumBytes());
}

void LConnection::rpcPause(U8 pnum, bool on)
{
    uint8_t buf[16];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_PAUSE));
    bs.write(pnum);
    bs.writeFlag(on);
    sendPacket(buf, bs.getNumBytes());
}

void LConnection::rpcMessage_s2c(S8 pnum, const char *msg, S8 priority, S8 type)
{
    uint8_t buf[512];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_MESSAGE_S2C));
    bs.write(pnum);
    bs.writeString(msg);
    bs.write(priority);
    bs.write(type);
    sendPacket(buf, bs.getNumBytes());
}

void LConnection::rpcSendNetVar_s2c(U16 netid, const char *str)
{
    uint8_t buf[512];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_NETVAR_S2C));
    bs.write(netid);
    bs.writeString(str);
    sendPacket(buf, bs.getNumBytes());
}

void LConnection::rpcKick_s2c(U8 pnum, const char *str)
{
    uint8_t buf[512];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_KICK_S2C));
    bs.write(pnum);
    bs.writeString(str);
    sendPacket(buf, bs.getNumBytes());
}

void LConnection::rpcSendOptions_c2s(U8 pnum, lnet::BitStream *buf)
{
    // TODO: implement
    (void)pnum;
    (void)buf;
}

void LConnection::rpcSuicide_c2s(U8 pnum)
{
    uint8_t buf[8];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_SUICIDE_C2S));
    bs.write(pnum);
    sendPacket(buf, bs.getNumBytes());
}

void LConnection::rpcRequestPOVchange_c2s(S32 pnum)
{
    uint8_t buf[16];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_POV_CHANGE_C2S));
    bs.write(pnum);
    sendPacket(buf, bs.getNumBytes());
}

void LConnection::rpcTiccmd_c2s(S32 pnum, const ticcmd_t *cmd, uint16_t seq)
{
    uint8_t buf[32];
    lnet::BitStream bs(buf, sizeof(buf));
    bs.write(U8(PKT_TICCMD_C2S));
    bs.write(pnum);  // player number
    bs.write(seq);    // sequence number for reliable delivery
    // Send only the essential movement fields
    bs.write(cmd->forward);   // forward/backward movement
    bs.write(cmd->side);      // strafe left/right
    bs.write(cmd->yaw);       // turn angle
    bs.write(cmd->buttons);   // buttons and weapon changes
    sendPacket(buf, bs.getNumBytes());
}

void LConnection::handlePacket(PacketType type, lnet::BitStream *stream)
{
    switch (type)
    {
        case PKT_TEST:
        {
            U8 num = stream->readUInt8();
            CONS_Printf("rpcTest received: %d\n", num);
        }
        break;

        case PKT_CHAT:
        {
            S8 from = stream->readInt8();
            S8 to = stream->readInt8();
            char msg[256];
            stream->readString(msg);
            // TODO: route to proper handler
            CONS_Printf("Chat from %d to %d: %s\n", from, to, msg);
        }
        break;

        case PKT_PAUSE:
        {
            U8 pnum = stream->readUInt8();
            bool on = stream->readFlag();
            // TODO: implement
            CONS_Printf("Pause request: pnum=%d, on=%d\n", pnum, on);
        }
        break;

        case PKT_TICCMD_C2S:
        {
            // Read ticcmd from client (with player number and sequence)
            S32 pnum;
            uint16_t seq;
            stream->read(&pnum);
            stream->read(&seq);
            ticcmd_t cmd;
            cmd.forward = stream->readInt8();
            cmd.side = stream->readInt8();
            cmd.yaw = stream->readInt16();
            cmd.buttons = stream->readUInt16();
            cmd.item = 0;
            cmd.pitch = 0;

            // Forward to server for processing (includes player number and sequence)
            if (netInterface)
                netInterface->OnReceiveTiccmd(this, pnum, &cmd, seq);
        }
        break;

        case PKT_GAME_STATE:
        {
            // Client receives authoritative game state from server
            uint32_t serverTic = stream->readUInt32();
            uint8_t numPlayers = stream->readUInt8();

            for (int i = 0; i < numPlayers; i++)
            {
                uint8_t pnum = stream->readUInt8();
                uint8_t hasMobj = stream->readUInt8();

                // Read position data into temp variables
                int32_t server_x = 0, server_y = 0, server_z = 0;
                uint32_t server_angle = 0;
                uint16_t stateIndex = 0;
                if (hasMobj)
                {
                    server_x = stream->readInt32();
                    server_y = stream->readInt32();
                    server_z = stream->readInt32();
                    server_angle = stream->readUInt32();
                    stateIndex = stream->readUInt16();
                }

                // Find local player info (this machine's players)
                LocalPlayerInfo *lp = nullptr;
                for (int j = 0; j < NUM_LOCALPLAYERS; j++)
                {
                    if (LocalPlayers[j].pnumber == pnum)
                    {
                        lp = &LocalPlayers[j];
                        break;
                    }
                }

                if (lp && lp->info && lp->info->pawn)
                {
                    // Local player: compare prediction with server state
                    if (lp->predicted_valid && hasMobj)
                    {
                        // Compute prediction error
                        int32_t dx = Abs(lp->predicted_x - server_x);
                        int32_t dy = Abs(lp->predicted_y - server_y);
                        int32_t dz = Abs(lp->predicted_z - server_z);
                        // Use predicted position if error is small (< 8 pixels = 8 << FRACBITS)
                        fixed_t ERROR_THRESHOLD = FRACUNIT * 8; // 8 pixel threshold
                        if (dx < ERROR_THRESHOLD && dy < ERROR_THRESHOLD && dz < ERROR_THRESHOLD)
                        {
                            // Prediction was good - use predicted position
                            lp->info->pawn->x = lp->predicted_x;
                            lp->info->pawn->y = lp->predicted_y;
                            lp->info->pawn->z = lp->predicted_z;
                            lp->info->pawn->angle = lp->predicted_angle;
                        }
                        else
                        {
                            // Prediction was wrong - snap to server
                            lp->info->pawn->x = server_x;
                            lp->info->pawn->y = server_y;
                            lp->info->pawn->z = server_z;
                            lp->info->pawn->angle = server_angle;
                        }
                    }
                    else if (hasMobj)
                    {
                        // No prediction yet: snap to server
                        lp->info->pawn->x = server_x;
                        lp->info->pawn->y = server_y;
                        lp->info->pawn->z = server_z;
                        lp->info->pawn->angle = server_angle;
                    }
                    // Update predicted state from server for next prediction
                    if (hasMobj)
                    {
                        lp->predicted_x = server_x;
                        lp->predicted_y = server_y;
                        lp->predicted_z = server_z;
                        lp->predicted_angle = server_angle;
                        lp->predicted_valid = true;
                    }
                }
                else
                {
                    // Remote player: set up interpolation to new position
                    PlayerInfo *remotePlayer = nullptr;
                    for (size_t k = 0; k < game.Players.size(); k++)
                    {
                        if (game.Players[k]->number == pnum)
                        {
                            remotePlayer = game.Players[k];
                            break;
                        }
                    }
                    if (remotePlayer && remotePlayer->pawn && hasMobj)
                    {
                        // Set up interpolation from current position to server position
                        // Save current position as start of interpolation
                        remotePlayer->interp_x = remotePlayer->pawn->x;
                        remotePlayer->interp_y = remotePlayer->pawn->y;
                        remotePlayer->interp_z = remotePlayer->pawn->z;
                        remotePlayer->interp_angle = remotePlayer->pawn->angle;

                        // Set target position
                        remotePlayer->target_x = server_x;
                        remotePlayer->target_y = server_y;
                        remotePlayer->target_z = server_z;
                        remotePlayer->target_angle = server_angle;

                        // Interpolation duration: 1 tic (will be refined by tic rate)
                        remotePlayer->interp_frac = 0;
                        remotePlayer->interp_tics = 1;
                        remotePlayer->interp_elapsed = 0;
                    }
                }
                (void)stateIndex; // State lookup would be needed for full implementation
            }
        }
        break;

        case PKT_DISCONNECT:
            onConnectionTerminated(ReasonSelfDisconnect, "Received disconnect");
            break;

        default:
            CONS_Printf("Unknown packet type: %d\n", type);
            break;
    }
}
