// Minimal TNL stub - no namespace conflicts with std
// Forward declarations only - no actual implementation

#ifndef TNL_STUB_H
#define TNL_STUB_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>

// Type aliases
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
typedef uint32_t IPAddress;
typedef unsigned char byte;

// Forward declarations
class BitStream;
class NetConnection;
class GhostConnection;
class NetObject;
class NetInterface;
class NetEvent;
class PacketStream;
class Address;
class Nonce;
class StringPtr;
class ByteBufferPtr;

// Simple stubs in global namespace
class BitStream {
public:
    BitStream() {}
    ~BitStream() {}

    void write(uint32_t val) {}
    uint32_t read() { return 0; }
    void writeFlag(bool val) {}
    bool readFlag() { return false; }
    void writeInt(uint32_t val, int bits) {}
    uint32_t readInt(int bits) { return 0; }

    void writeString(const char* str) {}
    void readString(char* buffer) {}
    void readString(char* buffer, size_t maxLen) {}
    void read(uint16_t* val) { *val = 0; }
    void read(int* val) { *val = 0; }
    void read(U32* val) { *val = 0; }
    void read(S32* val, size_t bits) { *val = 0; }
    void read(bool* val, size_t bits) { *val = false; }
    void read(byte* buffer, int bits) {}

    uint8_t* getBuffer() { return nullptr; }
    uint32_t getPosition() const { return 0; }
    void setPosition(uint32_t pos) {}
    uint32_t getRemainingBits() const { return 0; }
    bool isCompressed() const { return false; }
    void setCompressed(bool c) {}
    void clear() {}
    void alignBits() {}
    void writeBits(uint32_t val, int bits) {}
    uint32_t readBits(int bits) { return 0; }
};

class NetConnection {
public:
    NetConnection() {}
    virtual ~NetConnection() {}
    virtual void connect(const char* address, uint16_t port) {}
    virtual void disconnect() {}
    virtual bool isConnected() const { return false; }
    virtual void processPacket() {}
    virtual void sendPacket(const void* data, uint32_t size) {}
};

class GhostConnection : public NetConnection {
public:
    GhostConnection() {}
    virtual ~GhostConnection() {}
    void objectInScope(void* obj) {}
};

class NetObject {
public:
    NetObject() {}
    virtual ~NetObject() {}
    virtual bool onGhostAdd(GhostConnection *c) { return true; }
    virtual void onGhostRemove() {}
    virtual void performScopeQuery(GhostConnection *c) {}
};

class NetInterface {
public:
    NetInterface() {}
    NetInterface(const Address& addr) {}
    virtual ~NetInterface() {}
    void setAllowsConnections(bool allow) {}
};

class Address {
public:
    Address() {}
    Address(const char* str) {}
    std::string toString() const { return ""; }
    bool isEqualAddress(const Address& other) const { return false; }
    bool operator==(const Address& other) const { return false; }
};

class Nonce {
public:
    Nonce() {}
    void generate() {}
    void getRandom() {}
    void read(BitStream* s) {}
    void write(BitStream* s) {}
};

class StringPtr {
public:
    StringPtr() {}
    StringPtr(const char* s) {}
};

class ByteBufferPtr {
public:
    ByteBufferPtr() {}
};

class PacketStream : public BitStream {
public:
    PacketStream() {}
    void sendto(void* socket, const Address& addr) {}
};

class NetEvent {
public:
    NetEvent() {}
    virtual ~NetEvent() {}
};

// Enums
enum TerminationReason {
    ReasonNone = 0
};

// RPC functions
void s2cEnterMap(U8 mapnum) {}

// Macros
#define BIT(x) (1 << (x))
#define TNL_DECLARE_RPC(func, params) void func params {}
#define TNL_IMPLEMENT_CLASS(cls)
#define TNL_RPC_CONSTRUCT_NETEVENT(player, rpc, args) nullptr

U32 computeClientIdentityToken(const Address& addr, const Nonce& nonce) { return 0; }

// Packet type constants
enum {
    FirstValidInfoPacketId = 0,
    PT_ServerPing = FirstValidInfoPacketId
};

#endif // TNL_STUB_H
