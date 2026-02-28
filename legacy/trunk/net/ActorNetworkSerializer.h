#ifndef NET_ACTOR_NETWORK_SERIALIZER_H
#define NET_ACTOR_NETWORK_SERIALIZER_H

#include "network/INetworkSerializer.h"
#include "g_actor.h"

namespace DoomLegacy::Network
{

/**
 * @brief Network serializer for Actor objects
 *
 * Extracts serialization logic from Actor class, following SRP.
 */
class ActorNetworkSerializer : public INetworkSerializer
{
  public:
    void Serialize(void* obj, DoomLegacy::ISerializer& serializer, uint32_t mask) override;
    void Deserialize(void* obj, DoomLegacy::ISerializer& serializer, uint32_t mask) override;

  private:
    void SerializeMove(Actor* actor, DoomLegacy::ISerializer& s);
    void SerializePresentation(Actor* actor, DoomLegacy::ISerializer& s);
    void SerializeAnimation(Actor* actor, DoomLegacy::ISerializer& s);

    void DeserializeMove(Actor* actor, DoomLegacy::ISerializer& s);
    void DeserializePresentation(Actor* actor, DoomLegacy::ISerializer& s);
    void DeserializeAnimation(Actor* actor, DoomLegacy::ISerializer& s);
};

} // namespace DoomLegacy::Network

#endif // NET_ACTOR_NETWORK_SERIALIZER_H