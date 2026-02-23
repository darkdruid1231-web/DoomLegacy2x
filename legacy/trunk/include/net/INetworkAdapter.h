#ifndef INETWORKADAPTER_H
#define INETWORKADAPTER_H

#include <cstddef>
#include <cstdint>

/**
 * INetworkAdapter - Abstraction interface for network operations
 *
 * This interface decouples core code from specific networking libraries
 * (TNL, ENet, etc.). Core code depends on this abstraction rather than
 * concrete implementations.
 */
class INetworkAdapter
{
  public:
    virtual ~INetworkAdapter() = default;

    /**
     * Send a packet over the network
     * @param data Pointer to data to send
     * @param size Size of data in bytes
     */
    virtual void sendPacket(const void *data, size_t size) = 0;

    /**
     * Receive a packet from the network
     * @param buffer Buffer to store received data
     * @param size Maximum size of buffer
     * @return Number of bytes received (0 if no data available)
     */
    virtual size_t receivePacket(void *buffer, size_t size) = 0;

    /**
     * Check if currently connected to a peer
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() const = 0;
};

#endif // INETWORKADAPTER_H
