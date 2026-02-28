#ifndef NET_PLAYER_INFO_NETWORK_SERIALIZER_H
#define NET_PLAYER_INFO_NETWORK_SERIALIZER_H

#include "network/INetworkSerializer.h"
#include "g_player.h"

namespace DoomLegacy::Network
{

/**
 * @brief Network serializer for PlayerInfo objects
 *
 * Extracts serialization logic from PlayerInfo class.
 */
class PlayerInfoNetworkSerializer : public INetworkSerializer
{
  public:
    void Serialize(void* obj, DoomLegacy::ISerializer& serializer, uint32_t mask) override;
    void Deserialize(void* obj, DoomLegacy::ISerializer& serializer, uint32_t mask) override;

  private:
    void SerializeIdentity(PlayerInfo* player, DoomLegacy::ISerializer& s);
    void SerializePawn(PlayerInfo* player, DoomLegacy::ISerializer& s);
    void SerializeScore(PlayerInfo* player, DoomLegacy::ISerializer& s);
    void SerializePalette(PlayerInfo* player, DoomLegacy::ISerializer& s);
    void SerializeHudFlash(PlayerInfo* player, DoomLegacy::ISerializer& s);

    void DeserializeIdentity(PlayerInfo* player, DoomLegacy::ISerializer& s);
    void DeserializePawn(PlayerInfo* player, DoomLegacy::ISerializer& s);
    void DeserializeScore(PlayerInfo* player, DoomLegacy::ISerializer& s);
    void DeserializePalette(PlayerInfo* player, DoomLegacy::ISerializer& s);
    void DeserializeHudFlash(PlayerInfo* player, DoomLegacy::ISerializer& s);
};

} // namespace DoomLegacy::Network

#endif // NET_PLAYER_INFO_NETWORK_SERIALIZER_H