#include "TNLNetworkTransport.h"
#include "tnl/tnlBitStream.h"

namespace DoomLegacy::Network
{

TNLNetworkTransport::TNLNetworkTransport(TNL::NetInterface* netInterface)
    : netInterface_(netInterface)
{
}

void TNLNetworkTransport::SendMessage(uint32_t connectionId, const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType)
{
    // TODO: Implement using TNL BitStream
    // For now, stub implementation
    (void)connectionId;
    (void)serializer;
    (void)messageType;
}

void TNLNetworkTransport::BroadcastMessage(const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType)
{
    // TODO: Implement broadcast
    (void)serializer;
    (void)messageType;
}

bool TNLNetworkTransport::IsMultiplayer() const
{
    // TODO: Check actual multiplayer state
    return netInterface_ != nullptr;
}

} // namespace DoomLegacy::Network