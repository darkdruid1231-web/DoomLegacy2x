#ifndef TNL_NET_INTERFACE_H
#define TNL_NET_INTERFACE_H

#include "tnlBitStream.h"
#include "tnlNetBase.h"
#include <string>

// Forward declarations
class Address;
class Nonce;
class PacketStream;

// Stub NetInterface
class NetInterface
{
  public:
    NetInterface(const Address &bind) {}
    virtual ~NetInterface() {}

    virtual void handleInfoPacket(const Address &address, U8 packetType, BitStream *stream) {}
    virtual void checkIncomingPackets() {}
    virtual void processConnections() {}

    void setAllowsConnections(bool allow) {}
    bool getAllowsConnections() const { return false; }

    void disconnect(void *connection, int reason, const char *msg = nullptr) {}
};

// Stub Address
class Address
{
  public:
    Address() {}
    Address(U32 type, const char *addressString, U16 port) {}
    Address(U32 type, U32 address, U16 port) {}

    static const U32 Any = 0;

    bool operator==(const Address &other) const { return true; }
    bool operator!=(const Address &other) const { return false; }

    const char *toString() const { return "127.0.0.1:0"; }
};

// Stub Nonce
class Nonce
{
  public:
    Nonce() {}
    void getRandom() {}
    bool operator==(const Nonce &other) const { return true; }

    void read(BitStream *stream) {}
    void write(BitStream *stream) const {}
};

// Stub PacketStream
class PacketStream : public BitStream
{
  public:
    PacketStream() : BitStream() {}

    void sendto(void *socket, const Address &address) {}
};

#endif // TNL_NET_INTERFACE_H