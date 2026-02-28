# Doom Legacy Network Architecture Refactor

## Overview

This refactor decouples the networking code from core game logic, following SOLID principles, to prepare for ditching OpenTNL (Torque Networking Library).

## Key Changes

### 1. SOLID Principles Applied

- **Single Responsibility Principle (SRP)**: Separated serialization logic from Actor and PlayerInfo classes into dedicated serializers.
- **Open-Closed Principle (OCP)**: Used interfaces for network components, allowing extension without modification.
- **Liskov Substitution Principle (LSP)**: Interfaces ensure substitutable implementations.
- **Interface Segregation Principle (ISP)**: Focused interfaces for specific responsibilities.
- **Dependency Inversion Principle (DIP)**: Core code depends on abstractions, not concretions.

### 2. Abstractions Introduced

- `INetworkSerializer`: Interface for serializing game objects.
- `INetworkTransport`: Interface for network transport operations.
- `ISerializer`: Existing interface for data serialization (TNL-independent).

### 3. Dependency Injection

- `NetworkManager`: Central manager for injecting network components.
- Registers serializers and transport implementations.
- Provides access to network functionality.

### 4. Decoupled Serialization

- Moved pack/unpack logic from actors to `ActorNetworkSerializer` and `PlayerInfoNetworkSerializer`.
- Actors now delegate serialization to the NetworkManager.
- Supports multiple serialization strategies (TNL BitStream, ISerializer).

### 5. Multiplayer Isolation

- Network code is conditionally executed based on multiplayer state.
- Single-player mode bypasses network operations.
- Testability improved with mockable interfaces.

## Architecture Diagram

```
Core Game Logic (Actors, PlayerInfo)
    |
    | (depends on)
    v
INetworkSerializer <- NetworkManager -> INetworkTransport
    |                        |
    | (implements)           | (manages)
    v                        v
ActorNetworkSerializer    TNLNetworkTransport
PlayerInfoNetworkSerializer
    |
    | (uses)
    v
ISerializer <- BitStreamSerializer
```

## Benefits

- **Decoupling**: Core game logic no longer depends on TNL specifics.
- **Testability**: Interfaces allow for easy mocking in unit tests.
- **Maintainability**: Changes to networking don't affect game logic.
- **Extensibility**: Easy to add new serializers or transport implementations.
- **Future-Proofing**: Ready for migration away from TNL.

## Usage

### Initialization

```cpp
// Register serializers
NetworkManager::Instance().RegisterSerializer<Actor>(
    std::make_unique<ActorNetworkSerializer>());
NetworkManager::Instance().RegisterSerializer<PlayerInfo>(
    std::make_unique<PlayerInfoNetworkSerializer>());

// Set transport
NetworkManager::Instance().SetTransport(
    std::make_unique<TNLNetworkTransport>(netInterface));

// Enable multiplayer
NetworkManager::Instance().SetMultiplayer(true);
```

### Serialization

```cpp
// In Actor::packUpdate
auto* serializer = NetworkManager::Instance().GetSerializer<Actor>();
if (serializer) {
    BitStreamSerializer bsSerializer(stream);
    // Use serializer methods
}
```

### Testing

```cpp
// Mock implementations for unit tests
class MockNetworkSerializer : public INetworkSerializer {
    // Mock serialize/deserialize
};

class MockNetworkTransport : public INetworkTransport {
    // Mock send/broadcast
};
```

## Migration Path

1. Complete interface implementations.
2. Migrate all TNL BitStream usage to ISerializer.
3. Implement new transport layer (e.g., ENet).
4. Remove TNL dependencies.
5. Add comprehensive tests.

## Files Added/Modified

### New Interfaces
- `include/network/INetworkSerializer.h`
- `include/network/INetworkTransport.h`

### New Implementations
- `net/ActorNetworkSerializer.h/.cpp`
- `net/PlayerInfoNetworkSerializer.h/.cpp`
- `net/NetworkManager.h/.cpp`
- `net/TNLNetworkTransport.h/.cpp`
- `net/BitStreamSerializer.h/.cpp`

### Modified Files
- `engine/g_actor.cpp`: Updated packUpdate/unpackUpdate
- `engine/g_player.cpp`: Updated packUpdate/unpackUpdate

This architecture provides a clean separation of concerns, enabling easier maintenance, testing, and future networking library migrations.