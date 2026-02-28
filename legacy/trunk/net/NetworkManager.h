#ifndef NET_NETWORK_MANAGER_H
#define NET_NETWORK_MANAGER_H

#include "network/INetworkSerializer.h"
#include "network/INetworkTransport.h"
#include <memory>
#include <unordered_map>
#include <typeindex>

namespace DoomLegacy::Network
{

/**
 * @brief Central manager for network components
 *
 * Implements dependency injection for network serializers and transport.
 * Provides factory methods for serializers and manages multiplayer state.
 */
class NetworkManager
{
  public:
    static NetworkManager& Instance();

    // Serializer management
    template<typename T>
    void RegisterSerializer(std::unique_ptr<INetworkSerializer> serializer)
    {
        serializers_[typeid(T)] = std::move(serializer);
    }

    template<typename T>
    INetworkSerializer* GetSerializer()
    {
        auto it = serializers_.find(typeid(T));
        return it != serializers_.end() ? it->second.get() : nullptr;
    }

    // Transport management
    void SetTransport(std::unique_ptr<INetworkTransport> transport);
    INetworkTransport* GetTransport();

    // Multiplayer state
    void SetMultiplayer(bool enabled);
    bool IsMultiplayer() const;

    // Convenience methods for serialization
    template<typename T>
    void SerializeObject(T* obj, DoomLegacy::ISerializer& serializer, uint32_t mask)
    {
        if (auto* ser = GetSerializer<T>())
        {
            ser->Serialize(obj, serializer, mask);
        }
    }

    template<typename T>
    void DeserializeObject(T* obj, DoomLegacy::ISerializer& serializer, uint32_t mask)
    {
        if (auto* ser = GetSerializer<T>())
        {
            ser->Deserialize(obj, serializer, mask);
        }
    }

  private:
    NetworkManager() = default;
    ~NetworkManager() = default;
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    std::unordered_map<std::type_index, std::unique_ptr<INetworkSerializer>> serializers_;
    std::unique_ptr<INetworkTransport> transport_;
    bool multiplayer_ = false;
};

} // namespace DoomLegacy::Network

#endif // NET_NETWORK_MANAGER_H