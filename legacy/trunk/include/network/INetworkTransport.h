#ifndef NETWORK_INETWORK_TRANSPORT_H
#define NETWORK_INETWORK_TRANSPORT_H

#include "core/ISerializer.h"
#include <cstdint>

namespace DoomLegacy::Network
{

/**
 * @brief Interface for network transport operations
 *
 * This abstracts the transport layer, allowing different implementations
 * (TNL, ENet, etc.) and enabling dependency injection for testability.
 */
class INetworkTransport
{
  public:
    virtual ~INetworkTransport() = default;

    /**
     * @brief Send a message to a specific connection
     * @param connectionId Identifier for the connection
     * @param serializer The serialized message data
     * @param messageType Type of message (reliable, unreliable, etc.)
     */
    virtual void SendMessage(uint32_t connectionId, const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType) = 0;

    /**
     * @brief Broadcast a message to all connections
     * @param serializer The serialized message data
     * @param messageType Type of message
     */
    virtual void BroadcastMessage(const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType) = 0;

    /**
     * @brief Check if currently in multiplayer mode
     * @return true if multiplayer is active
     */
    virtual bool IsMultiplayer() const = 0;
};

} // namespace DoomLegacy::Network

#endif // NETWORK_INETWORK_TRANSPORT_H