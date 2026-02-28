#ifndef NET_ENET_NETWORK_TRANSPORT_H
#define NET_ENET_NETWORK_TRANSPORT_H

#include "network/INetworkTransport.h"
#include "core/ISerializer.h"
#include <enet/enet.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace DoomLegacy::Network
{

/**
 * @brief ENet-based implementation of INetworkTransport
 *
 * This transport uses ENet for reliable UDP networking, providing
 * connection management, packet sending/receiving, and serialization.
 */
class ENetNetworkTransport : public INetworkTransport
{
  public:
    /**
     * @brief Constructor
     * @param port Port to listen on (0 for client mode)
     * @param maxConnections Maximum number of connections
     */
    ENetNetworkTransport(uint16_t port = 0, size_t maxConnections = 32);
    virtual ~ENetNetworkTransport();

    // INetworkTransport interface
    void SendMessage(uint32_t connectionId, const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType) override;
    void BroadcastMessage(const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType) override;
    bool IsMultiplayer() const override;

    /**
     * @brief Initialize the transport
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Connect to a server
     * @param address Server address
     * @param port Server port
     * @return Connection ID or 0 on failure
     */
    uint32_t Connect(const char* address, uint16_t port);

    /**
     * @brief Disconnect a connection
     * @param connectionId Connection to disconnect
     */
    void Disconnect(uint32_t connectionId);

    /**
     * @brief Service the network (must be called regularly)
     * @param timeoutMs Timeout in milliseconds
     * @return true if there was network activity
     */
    bool Service(uint32_t timeoutMs = 0);

    /**
     * @brief Set callback for received messages
     * @param callback Function to call when a message is received
     */
    void SetReceiveCallback(std::function<void(uint32_t connectionId, std::unique_ptr<DoomLegacy::ISerializer>)> callback);

    /**
     * @brief Set callback for connection events
     * @param connectCallback Called when a client connects (server only)
     * @param disconnectCallback Called when a client disconnects
     */
    void SetConnectionCallbacks(
        std::function<void(uint32_t connectionId)> connectCallback,
        std::function<void(uint32_t connectionId)> disconnectCallback);

  private:
    ENetHost* host_;
    uint16_t port_;
    size_t maxConnections_;

    // Connection management
    std::unordered_map<uint32_t, ENetPeer*> connections_;
    std::unordered_map<ENetPeer*, uint32_t> peerToId_;
    uint32_t nextConnectionId_;

    // Callbacks
    std::function<void(uint32_t, std::unique_ptr<DoomLegacy::ISerializer>)> receiveCallback_;
    std::function<void(uint32_t)> connectCallback_;
    std::function<void(uint32_t)> disconnectCallback_;

    // Convert MessageType to ENet flags
    uint32_t MessageTypeToENetFlags(DoomLegacy::MessageType type) const;

    // Create connection ID
    uint32_t CreateConnectionId();
};

} // namespace DoomLegacy::Network

#endif // NET_ENET_NETWORK_TRANSPORT_H