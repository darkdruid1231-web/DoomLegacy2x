#ifndef NETWORK_INETWORK_SERIALIZER_H
#define NETWORK_INETWORK_SERIALIZER_H

#include "core/ISerializer.h"

namespace DoomLegacy::Network
{

/**
 * @brief Interface for network serialization of specific game objects
 *
 * This follows the Strategy pattern to decouple serialization logic from
 * the objects being serialized, allowing for different serialization
 * strategies (TNL BitStream, ISerializer, etc.) and testability with mocks.
 */
class INetworkSerializer
{
  public:
    virtual ~INetworkSerializer() = default;

    /**
     * @brief Serialize an object for network transmission
     * @param obj The object to serialize (e.g., Actor*, PlayerInfo*)
     * @param serializer The serializer to use
     * @param mask Update mask indicating what to serialize
     */
    virtual void Serialize(void* obj, DoomLegacy::ISerializer& serializer, uint32_t mask) = 0;

    /**
     * @brief Deserialize an object from network data
     * @param obj The object to deserialize into
     * @param serializer The serializer to use
     * @param mask Update mask indicating what to deserialize
     */
    virtual void Deserialize(void* obj, DoomLegacy::ISerializer& serializer, uint32_t mask) = 0;
};

} // namespace DoomLegacy::Network

#endif // NETWORK_INETWORK_SERIALIZER_H