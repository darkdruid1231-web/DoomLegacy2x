#ifndef STUBNETWORKADAPTER_H
#define STUBNETWORKADAPTER_H

#include "INetworkAdapter.h"

/**
 * StubNetworkAdapter - Null implementation for testing
 *
 * This adapter provides a no-op implementation of INetworkAdapter
 * for unit tests and environments where networking is not available.
 * All operations are effectively no-ops:
 * - sendPacket: discards data
 * - receivePacket: returns 0 (no data)
 * - isConnected: returns false
 */
class StubNetworkAdapter : public INetworkAdapter
{
  public:
    StubNetworkAdapter() = default;
    virtual ~StubNetworkAdapter() = default;

    /**
     * Discard packet data (no-op)
     */
    virtual void sendPacket(const void *data, size_t size) override
    {
        (void)data;
        (void)size;
        // Intentionally empty - discards all data
    }

    /**
     * Returns 0 bytes (no data available)
     */
    virtual size_t receivePacket(void *buffer, size_t size) override
    {
        (void)buffer;
        (void)size;
        return 0; // No data available
    }

    /**
     * Always returns false (not connected)
     */
    virtual bool isConnected() const override
    {
        return false;
    }
};

#endif // STUBNETWORKADAPTER_H
