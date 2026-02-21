#ifndef TNL_NET_INTERFACE_H
#define TNL_NET_INTERFACE_H

#include "tnlBitStream.h"
#include "tnlNetBase.h"
#include <cstdint>
#include <string>
#include <vector>

namespace TNL {

class NetInterface;
class NetConnection;
class NetObject;

// Address stub
class Address {
public:
    Address() : port(0) {}
    Address(const char* ip, uint16_t p) : ip(ip), port(p) {}
    std::string ip;
    uint16_t port;
};

// Nonce stub
class Nonce {
public:
    Nonce() {}
    Nonce(const uint8_t* data, size_t len) {}
};

// NetClassRep for RTTI
class NetClassRep {
public:
    NetClassRep() : className(nullptr), group(0), bit(0) {}
    const char* className;
    U32 group;
    U32 bit;
};

class NetConnection {
public:
    NetConnection() : mInterface(nullptr), mState(0), mConnectId(0) {}
    virtual ~NetConnection() {}
    
    enum PacketType {
        PT_ServerPing = 0,
        FirstValidInfoPacketId = 100
    };
    
    virtual void connect(const char* address, uint16_t port) {}
    virtual void disconnect() {}
    virtual void update() {}
    
    virtual void sendPacket(BitStream* stream, uint8_t packetType) {}
    virtual void onPacketReceived(uint8_t packetType, BitStream* stream) {}
    
    virtual void onConnect() {}
    virtual void onDisconnect() {}
    
    int getState() const { return mState; }
    void setState(int s) { mState = s; }
    
    NetInterface* getInterface() { return mInterface; }
    void setInterface(NetInterface* iface) { mInterface = iface; }
    
    uint32_t getConnectId() const { return mConnectId; }
    void setConnectId(uint32_t id) { mConnectId = id; }
    
private:
    NetInterface* mInterface;
    int mState;
    uint32_t mConnectId;
};

class NetObject {
public:
    NetObject() : mNetInterface(nullptr), mGhostIndex(0) {}
    virtual ~NetObject() {}
    
    virtual void onGhostAdded() {}
    virtual void onGhostRemoved() {}
    virtual void onGhostUpdate(BitStream* stream) {}
    virtual void onGhostReceive(BitStream* stream) {}
    
    virtual void packUpdate(BitStream* stream) {}
    virtual bool unpackUpdate(BitStream* stream) { return true; }
    
    NetInterface* getNetInterface() { return mNetInterface; }
    void setNetInterface(NetInterface* iface) { mNetInterface = iface; }
    
    int getGhostIndex() const { return mGhostIndex; }
    void setGhostIndex(int idx) { mGhostIndex = idx; }
    
private:
    NetInterface* mNetInterface;
    int mGhostIndex;
};

class NetInterface {
public:
    NetInterface() {}
    virtual ~NetInterface() {}
    
    virtual bool connect(const char* address, uint16_t port) { return false; }
    virtual void disconnect() {}
    virtual void update() {}
    
    virtual void send(NetConnection* conn, BitStream* stream, uint8_t packetType) {}
    virtual void processPacket(NetConnection* conn, uint8_t packetType, BitStream* stream) {}
    
    virtual void onConnect(NetConnection* conn) {}
    virtual void onDisconnect(NetConnection* conn) {}
    virtual void onConnectionRejected(NetConnection* conn, const char* reason) {}
    
    void setMaxConnections(int max) {}
    int getMaxConnections() const { return 0; }
    
    NetConnection* getConnection(int index) { return nullptr; }
    int getConnectionCount() const { return 0; }
};

} // namespace TNL

#endif // TNL_NET_INTERFACE_H
