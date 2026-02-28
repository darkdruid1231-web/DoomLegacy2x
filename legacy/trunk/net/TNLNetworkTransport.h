#ifndef NET_TNL_NETWORK_TRANSPORT_H
#define NET_TNL_NETWORK_TRANSPORT_H

#include "network/INetworkTransport.h"
#include "tnl/tnlNetInterface.h"

namespace DoomLegacy::Network
{

/**
 * @brief TNL-based implementation of INetworkTransport
 *
 * Wraps TNL networking functionality, allowing dependency injection
 * and testability with mocks.
 */
class TNLNetworkTransport : public INetworkTransport
{
  public:
    explicit TNLNetworkTransport(TNL::NetInterface* netInterface = nullptr);

    void SendMessage(uint32_t connectionId, const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType) override;
    void BroadcastMessage(const DoomLegacy::ISerializer& serializer, DoomLegacy::MessageType messageType) override;
    bool IsMultiplayer() const override;

  private:
    TNL::NetInterface* netInterface_;
};

} // namespace DoomLegacy::Network

#endif // NET_TNL_NETWORK_TRANSPORT_H