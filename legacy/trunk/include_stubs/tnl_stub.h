// Comprehensive TNL stub header for build compatibility
#ifndef TNL_STUB_H
#define TNL_STUB_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>

// Basic types
namespace TNL {
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;
typedef float F32;
typedef double F64;

// BIT macro
#define BIT(n) (1 << (n))

// Assert stubs
#define TNLAssert(expr, msg)
#define TNLAssertMacro(expr, msg)

inline void assertHandler(const char* expr, const char* file, int line, const char* msg) {
    std::fprintf(stderr, "Assertion failed: %s at %s:%d\n", expr, file, line);
}

// NetClassRep
class NetClassRep {
public:
    const char* className;
    U32 group;
    U32 bit;
    NetClassRep() : className(nullptr), group(0), bit(0) {}
};

// BitStream stub
class BitStream {
public:
    BitStream() : buffer(), currentBit(0), ownBuffer(true) {}
    explicit BitStream(size_t size) : buffer(size), currentBit(0), ownBuffer(true) {}
    BitStream(void* data, size_t size, bool own) : buffer(), currentBit(0), ownBuffer(own) {
        if (data && size) {
            buffer.assign(static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size);
        }
    }
    ~BitStream() {}
    
    // Write methods
    void write(const void* data, size_t bits) {
        const uint8_t* src = static_cast<const uint8_t*>(data);
        size_t bytes = (bits + 7) >> 3;
        for (size_t i = 0; i < bytes; i++) {
            buffer.push_back(src[i]);
        }
    }
    void write(uint8_t val) { write(&val, 8); }
    void write(int8_t val) { write(&val, 8); }
    void write(uint16_t val) { write(&val, 16); }
    void write(int16_t val) { write(&val, 16); }
    void write(uint32_t val) { write(&val, 32); }
    void write(int32_t val) { write(&val, 32); }
    void writeFloat(float val) { write(&val, 32); }
    void writeString(const char* str) {
        if (str) {
            size_t len = strlen(str) + 1;
            writeUInt32(len, 16);
            write(str, len * 8);
        } else {
            writeUInt32(0, 16);
        }
    }
    void writeInt(int32_t value, int bits) { write(&value, bits); }
    void writeUInt(uint32_t value, int bits) { write(&value, bits); }
    void writeUInt32(uint32_t val, int bits = 32) { write(&val, bits); }
    void writeBool(bool val) { write(&val, 1); }
    
    // Read methods  
    void read(void* data, size_t bits) {
        uint8_t* dest = static_cast<uint8_t*>(data);
        size_t bytes = (bits + 7) >> 3;
        for (size_t i = 0; i < bytes && (currentBit >> 3) < buffer.size(); i++) {
            dest[i] = buffer[currentBit >> 3];
            currentBit += 8;
        }
    }
    template<typename T>
    void read(T* dest) { read(dest, sizeof(T) * 8); }
    uint8_t readUInt8() { uint8_t v; read(&v, 8); return v; }
    int8_t readInt8() { int8_t v; read(&v, 8); return v; }
    uint16_t readUInt16() { uint16_t v; read(&v, 16); return v; }
    int16_t readInt16() { int16_t v; read(&v, 16); return v; }
    uint32_t readUInt32() { uint32_t v; read(&v, 32); return v; }
    int32_t readInt32() { int32_t v; read(&v, 32); return v; }
    float readFloat() { float v; read(&v, 32); return v; }
    int32_t readInt(int bits) { int32_t v; read(&v, bits); return v; }
    uint32_t readUInt(int bits) { uint32_t v; read(&v, bits); return v; }
    void readString(std::string& str) {
        uint32_t len = readUInt32();
        if (len > 0 && len < 65536) {
            std::vector<char> buf(len);
            read(buf.data(), len * 8);
            str = buf.data();
        } else {
            str.clear();
        }
    }
    void readString(char* dest) {
        uint32_t len = readUInt32();
        if (len > 0 && len < 65536) {
            read(dest, len * 8);
        }
    }
    bool readBool() { bool v; read(&v, 1); return v; }
    
    // Position
    void setBitPosition(size_t bit) { currentBit = bit; }
    size_t getBitPosition() const { return currentBit; }
    void setBytePosition(size_t pos) { currentBit = pos * 8; }
    size_t getBytePosition() const { return currentBit >> 3; }
    void clear() { buffer.clear(); currentBit = 0; }
    void reset() { currentBit = 0; }
    bool isAtEnd() const { return (currentBit >> 3) >= buffer.size(); }
    uint8_t* getBuffer() { return buffer.data(); }
    const uint8_t* getBuffer() const { return buffer.data(); }
    
private:
    std::vector<uint8_t> buffer;
    size_t currentBit;
    bool ownBuffer;
};

// Forward declarations
class NetInterface;
class NetObject;
class NetConnection;

// Address stub
class Address {
public:
    Address() : port(0) {}
    Address(const char* ip, uint16_t p) : ipStr(ip), port(p) {}
    std::string ipStr;
    uint16_t port;
};

// Nonce stub
class Nonce {
public:
    Nonce() {}
    Nonce(const uint8_t*, size_t) {}
};

// Packet types
const U32 FirstValidInfoPacketId = 100;

// NetConnection stub
class NetConnection {
public:
    NetConnection() : mInterface(nullptr), mState(0), mConnectId(0) {}
    virtual ~NetConnection() {}
    enum PacketType { PT_ServerPing = 0 };
    virtual void connect(const char*, uint16_t) {}
    virtual void disconnect() {}
    virtual void update() {}
    virtual void sendPacket(BitStream*, uint8_t) {}
    virtual void onPacketReceived(uint8_t, BitStream*) {}
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

// NetObject stub
class NetObject {
public:
    NetObject() : mNetInterface(nullptr), mGhostIndex(0) {}
    virtual ~NetObject() {}
    virtual void onGhostAdded() {}
    virtual void onGhostRemoved() {}
    virtual void onGhostUpdate(BitStream*) {}
    virtual void onGhostReceive(BitStream*) {}
    virtual void packUpdate(BitStream*) {}
    virtual bool unpackUpdate(BitStream*) { return true; }
    NetInterface* getNetInterface() { return mNetInterface; }
    void setNetInterface(NetInterface* iface) { mNetInterface = iface; }
    int getGhostIndex() const { return mGhostIndex; }
    void setGhostIndex(int idx) { mGhostIndex = idx; }
private:
    NetInterface* mNetInterface;
    int mGhostIndex;
};

// NetInterface stub
class NetInterface {
public:
    NetInterface() {}
    virtual ~NetInterface() {}
    virtual bool connect(const char*, uint16_t) { return false; }
    virtual void disconnect() {}
    virtual void update() {}
    virtual void send(NetConnection*, BitStream*, uint8_t) {}
    virtual void processPacket(NetConnection*, uint8_t, BitStream*) {}
    virtual void onConnect(NetConnection*) {}
    virtual void onDisconnect(NetConnection*) {}
    virtual void onConnectionRejected(NetConnection*, const char*) {}
    void setMaxConnections(int) {}
    int getMaxConnections() const { return 0; }
    NetConnection* getConnection(int) { return nullptr; }
    int getConnectionCount() const { return 0; }
};

// GhostConnection stub
class GhostConnection : public NetConnection {
public:
    GhostConnection() {}
    virtual ~GhostConnection() {}
    void addGhost(NetObject*) {}
    void removeGhost(NetObject*) {}
    void updateGhost(NetObject*) {}
    virtual void onGhostAdded(NetObject*) {}
    virtual void onGhostRemoved(NetObject*) {}
    virtual void onGhostUpdate(NetObject*) {}
};

// RPC stubs
#define TNL_DECLARE_RPC(name, params)
#define TNL_IMPLEMENT_RPC(cls, name, params)
#define TNL_DECLARE_NET_EVENT(cls)

} // namespace TNL

#endif // TNL_STUB_H
