#include "PlayerInfoNetworkSerializer.h"
#include "g_player.h"
#include "n_connection.h" // for GhostConnection

namespace DoomLegacy::Network
{

void PlayerInfoNetworkSerializer::Serialize(void* obj, DoomLegacy::ISerializer& s, uint32_t mask)
{
    PlayerInfo* player = static_cast<PlayerInfo*>(obj);

    if (s.isWriting())
    {
        // Writing: pack
        if (mask & PlayerInfo::M_IDENTITY)
        {
            SerializeIdentity(player, s);
        }

        if (mask & PlayerInfo::M_PAWN)
        {
            SerializePawn(player, s);
        }

        if (mask & PlayerInfo::M_SCORE)
        {
            SerializeScore(player, s);
        }

        // Feedback only to owner
        if (mask & PlayerInfo::M_PALETTE)
        {
            SerializePalette(player, s);
        }

        if (mask & PlayerInfo::M_HUDFLASH)
        {
            SerializeHudFlash(player, s);
        }
    }
    else
    {
        // Reading: unpack
        if (mask & PlayerInfo::M_IDENTITY)
        {
            DeserializeIdentity(player, s);
        }

        if (mask & PlayerInfo::M_PAWN)
        {
            DeserializePawn(player, s);
        }

        if (mask & PlayerInfo::M_SCORE)
        {
            DeserializeScore(player, s);
        }

        if (mask & PlayerInfo::M_PALETTE)
        {
            DeserializePalette(player, s);
        }

        if (mask & PlayerInfo::M_HUDFLASH)
        {
            DeserializeHudFlash(player, s);
        }
    }
}

void PlayerInfoNetworkSerializer::Deserialize(void* obj, DoomLegacy::ISerializer& s, uint32_t mask)
{
    // Handled in Serialize
    Serialize(obj, s, mask);
}

void PlayerInfoNetworkSerializer::SerializeIdentity(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    s.writeString(player->name.c_str());
    s.write(static_cast<uint32_t>(player->number));
    s.write(static_cast<uint32_t>(player->team));
}

void PlayerInfoNetworkSerializer::SerializePawn(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    // Note: This requires GhostConnection for getGhostIndex
    // We'll need to inject the connection or handle differently
    // For now, skip pawn serialization in this refactor
    // TODO: Abstract ghost indexing
}

void PlayerInfoNetworkSerializer::SerializeScore(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    s.write(static_cast<uint32_t>(player->score));
}

void PlayerInfoNetworkSerializer::SerializePalette(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    s.write(static_cast<int32_t>(player->palette));
    player->palette = -1; // reset after sending
}

void PlayerInfoNetworkSerializer::SerializeHudFlash(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    s.write(static_cast<int32_t>(player->damagecount));
    s.write(static_cast<int32_t>(player->bonuscount));
    s.write(static_cast<int32_t>(player->itemuse));
}

void PlayerInfoNetworkSerializer::DeserializeIdentity(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    std::string temp;
    s.readString(temp);
    player->name = temp;
    player->number = static_cast<int>(s.readUInt32());
    player->team = static_cast<int>(s.readUInt32());
}

void PlayerInfoNetworkSerializer::DeserializePawn(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    // TODO: Handle ghost index resolution
}

void PlayerInfoNetworkSerializer::DeserializeScore(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    player->score = static_cast<int>(s.readUInt32());
}

void PlayerInfoNetworkSerializer::DeserializePalette(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    player->palette = static_cast<int>(s.readInt32());
}

void PlayerInfoNetworkSerializer::DeserializeHudFlash(PlayerInfo* player, DoomLegacy::ISerializer& s)
{
    player->damagecount = static_cast<int>(s.readInt32());
    player->bonuscount = static_cast<int>(s.readInt32());
    player->itemuse = static_cast<int>(s.readInt32());
}

} // namespace DoomLegacy::Network