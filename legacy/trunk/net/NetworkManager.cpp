#include "NetworkManager.h"

namespace DoomLegacy::Network
{

NetworkManager& NetworkManager::Instance()
{
    static NetworkManager instance;
    return instance;
}

void NetworkManager::SetTransport(std::unique_ptr<INetworkTransport> transport)
{
    transport_ = std::move(transport);
}

INetworkTransport* NetworkManager::GetTransport()
{
    return transport_.get();
}

void NetworkManager::SetMultiplayer(bool enabled)
{
    multiplayer_ = enabled;
}

bool NetworkManager::IsMultiplayer() const
{
    return multiplayer_;
}

} // namespace DoomLegacy::Network