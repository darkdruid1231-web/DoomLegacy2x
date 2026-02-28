#include "ENetNetworkTransport.h"
#include "core/ISerializer.h"
#include <cstring>
#include <iostream>

namespace DoomLegacy::Network
{

ENetNetworkTransport::ENetNetworkTransport(uint16_t port, size_t maxConnections)
    : host_(nullptr), port_(port), maxConnections_(maxConnections),
      nextConnectionId_(1), receiveCallback_(nullptr), connectCallback_(nullptr), disconnectCallback_(nullptr)
{
    if (enet_initialize() != 0)
    {
        std::cerr << "ENet initialization failed" << std::endl;
    }
}

ENetNetworkTransport::~ENetNetworkTransport()
{
    if (host_)
    {
        enet_host_destroy(host_);
    }
    enet_deinitialize();
}

bool ENetNetworkTransport::Initialize()
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port_;

    host_ = enet_host_create(port_ == 0 ? nullptr : &address, maxConnections_, 2, 0, 0);
    if (!host_)
    {
        std::cerr << "Failed to create ENet host" << std::endl;
        return false;
    }

    return true;
}

uint32_t ENetNetworkTransport::Connect(const char* address, uint16_t port)
{
    if (!host_)
        return 0;

    ENetAddress enetAddress;
    enet_address_set_host(&enetAddress, address);
    enetAddress.port = port;

    ENetPeer* peer = enet_host_connect(host_, &enetAddress, 2, 0);
    if (!peer)
        return 0;

    uint32_t connectionId = CreateConnectionId();
    connections_[connectionId] = peer;
    peerToId_[peer] = connectionId;

    return connectionId;
}

void ENetNetworkTransport::Disconnect(uint32_t connectionId)
{
    auto it = connections_.find(connectionId);
    if (it != connections_.end())
    {
        enet_peer_disconnect(it->second, 0);
        connections_.erase(it);
        peerToId_.erase(it->second);
    }
}

bool ENetNetworkTransport::Service(uint32_t timeoutMs)
{
    if (!host_)
        return false;

    ENetEvent event;
    bool hadEvent = false;

    while (enet_host_service(host_, &event, timeoutMs) > 0)
    {
        hadEvent = true;
        timeoutMs = 0; // Don't wait on subsequent iterations

        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                uint32_t connectionId = CreateConnectionId();
                connections_[connectionId] = event.peer;
                peerToId_[event.peer] = connectionId;

                if (connectCallback_)
                    connectCallback_(connectionId);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                auto it = peerToId_.find(event.peer);
                if (it != peerToId_.end() && receiveCallback_)
                {
                    // Create serializer from packet data
                    auto serializer = std::make_unique<DoomLegacy::InMemorySerializer>();
                    serializer->getBuffer().assign(event.packet->data, event.packet->data + event.packet->dataLength);
                    serializer->rewindForReading();

                    receiveCallback_(it->second, std::move(serializer));
                }

                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                auto it = peerToId_.find(event.peer);
                if (it != peerToId_.end())
                {
                    uint32_t connectionId = it->second;
                    connections_.erase(connectionId);
                    peerToId_.erase(it);

                    if (disconnectCallback_)
                        disconnectCallback_(connectionId);
                }
                break;
            }

            default:
                break;
        }
    }

    return hadEvent;
}

void ENetNetworkTransport::SendMessage(uint32_t connectionId, const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType)
{
    auto it = connections_.find(connectionId);
    if (it == connections_.end())
        return;

    const auto& buffer = static_cast<const DoomLegacy::InMemorySerializer&>(serializer).getBuffer();
    if (buffer.empty())
        return;

    ENetPacket* packet = enet_packet_create(buffer.data(), buffer.size(), MessageTypeToENetFlags(messageType));
    if (packet)
    {
        enet_peer_send(it->second, 0, packet);
    }
}

void ENetNetworkTransport::BroadcastMessage(const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType)
{
    const auto& buffer = static_cast<const DoomLegacy::InMemorySerializer&>(serializer).getBuffer();
    if (buffer.empty())
        return;

    ENetPacket* packet = enet_packet_create(buffer.data(), buffer.size(), MessageTypeToENetFlags(messageType));
    if (packet)
    {
        enet_host_broadcast(host_, 0, packet);
    }
}

bool ENetNetworkTransport::IsMultiplayer() const
{
    return !connections_.empty();
}

uint32_t ENetNetworkTransport::MessageTypeToENetFlags(DoomLegacy::MessageType type) const
{
    switch (type)
    {
        case DoomLegacy::MessageType::Reliable:
            return ENET_PACKET_FLAG_RELIABLE;
        case DoomLegacy::MessageType::Unreliable:
            return 0;
        case DoomLegacy::MessageType::Ordered:
            return ENET_PACKET_FLAG_RELIABLE; // ENet reliable is ordered
        default:
            return ENET_PACKET_FLAG_RELIABLE;
    }
}

uint32_t ENetNetworkTransport::CreateConnectionId()
{
    return nextConnectionId_++;
}

void ENetNetworkTransport::SetReceiveCallback(std::function<void(uint32_t, std::unique_ptr<DoomLegacy::ISerializer>)> callback)
{
    receiveCallback_ = callback;
}

void ENetNetworkTransport::SetConnectionCallbacks(
    std::function<void(uint32_t)> connectCallback,
    std::function<void(uint32_t)> disconnectCallback)
{
    connectCallback_ = connectCallback;
    disconnectCallback_ = disconnectCallback;
}

} // namespace DoomLegacy::Network