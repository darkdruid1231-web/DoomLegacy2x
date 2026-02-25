// Minimal TNL stub - no namespace conflicts with std
// Forward declarations only - no actual implementation
// Updated for CI fix

#ifndef TNL_STUB_H
#define TNL_STUB_H

#include <cstdint>
#include <string>
#include <vector>
#ifdef SDL2
#include <SDL2/SDL.h>
#ifdef HAVE_SDL2_MIXER
#include <SDL2/SDL_mixer.h>
#endif
#else
#include <SDL/SDL.h>
#ifdef HAVE_SDL2_MIXER
#include <SDL/SDL_mixer.h>
#endif
#endif
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
#ifdef TNL_STUB_BUILD
class BitStream;
#else
using lnet::BitStream;
#endif
class NetConnection;
class GhostConnection;
class NetObject;
class NetInterface;
class NetEvent;
class PacketStream;
class Address;
class Nonce;
class StringPtr;
typedef BitStream* ByteBufferPtr;

// Macros
#define BIT(x) (1 << (x))
#ifdef TNL_STUB_BUILD
#define TNL_DECLARE_RPC(func, params) void func params {}
#else
#define TNL_DECLARE_RPC(func, params) void func params
#endif
#define TNL_IMPLEMENT_CLASS(cls)
#define TNL_IMPLEMENT_NETOBJECT(cls)
#define TNL_IMPLEMENT_NETCONNECTION(cls, group, flag)
#define TNL_IMPLEMENT_NETOBJECT_RPC(cls, method, args, ...) void cls::method args {}
#define TNL_RPC_CONSTRUCT_NETEVENT(player, rpc, args) nullptr

// Enums
enum TransportProtocol {
    IPProtocol,
    IPXProtocol
};

enum NetClassGroup {
    NetClassGroupGame
};

// Packet type constants
enum {
    FirstValidInfoPacketId = 0,
    PT_ServerPing = FirstValidInfoPacketId,
    PT_PingResponse,
    PT_ServerQuery,
    PT_QueryResponse
};

// Networking constants
enum {
    NetClassGroupGameMask = 0,
    RPCGuaranteedOrdered = 0,
    RPCToGhost = 0,
    RPCToGhostParent = 0
};

enum ConnectionStateEnum {
    StateNotConnected = 0,
    StateAwaitingChallengeResponse,
    StateSendingPunchPackets,
    StateComputingPuzzleSolution,
    StateAwaitingConnectResponse,
    StateConnectTimeout,
    StateConnectionRejected,
    StateConnected,
    StateDisconnected,
    StateConnectionTimedOut
};

class Address {
public:
    Address() {}
    Address(const char* str) {}
    Address(TransportProtocol ptc, Address (*func)(), S32 port) {}  // Match the call
    std::string toString() const { return ""; }  // Changed from toString() to getNetAddressString() if needed
    bool isEqualAddress(const Address& other) const { return false; }
    bool operator==(const Address& other) const { return false; }
    bool operator!=(const Address& other) const { return !(*this == other); }
    U32 hash() const { return 0; }

    static Address Any() { return Address("0.0.0.0"); }
    static Address Broadcast() { return Address("255.255.255.255"); }
};

class Socket {};  // Stub for Socket if needed

class Nonce {
public:
    Nonce() {}
    void generate() {}
    void getRandom() {}
    void read(BitStream* s) {}
    void write(BitStream* s) {}
    bool operator==(const Nonce& other) const { return false; }
    bool operator!=(const Nonce& other) const { return !(*this == other); }
};

#ifdef TNL_STUB_BUILD
class BitStream {
public:
    BitStream() {}
    BitStream(U8* buffer, U32 size) {}
    ~BitStream() {}

    void write(uint32_t val) {}
    void write(int size, const byte* data) {}
    uint32_t read() { return 0; }
    bool writeFlag(bool val) { return val; }
    bool readFlag() { return false; }
    void writeInt(uint32_t val, int bits) {}
    uint32_t readInt(int bits) { return 0; }

    void writeString(const char* str) {}
    void readString(char* buffer) {}
    void readString(char* buffer, size_t maxLen) {}
    void readString(char* buffer, int maxLen) {}
    void read(uint16_t* val) { *val = 0; }
    void read(int* val) { *val = 0; }
    void read(U32* val) { *val = 0; }
    void read(S32* val, size_t bits) { *val = 0; }
    void read(bool* val, size_t bits) { *val = false; }
    void read(byte* buffer, int bits) {}
    void read(unsigned char* val) { *val = 0; }
    void read(bool* val) { *val = false; }

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
    U32 getBufferSize() const { return 0; }
    U32 getBytePosition() const { return 0; }
};
#endif

class PacketStream : public BitStream {
public:
    PacketStream() {}
    void sendto(void* socket, const Address& addr) {}
};

class StringPtr {
public:
    StringPtr() {}
    StringPtr(const char* s) {}
    const char* getString() const { return ""; }
};

class NetEvent {
public:
    NetEvent() {}
    virtual ~NetEvent() {}
};

class NetObject {
public:
    NetObject() {}
    virtual ~NetObject() {}
    virtual bool onGhostAdd(GhostConnection *c) { return true; }
    virtual void onGhostRemove() {}
    virtual void performScopeQuery(GhostConnection *c) {}
    void setMaskBits(U32 mask) {}
};

class NetConnection {
public:
    enum TerminationReason {
        ReasonNone = 0,
        ReasonSelfDisconnect
    };

    NetConnection() {}
    virtual ~NetConnection() {}
    virtual void connect(NetInterface* iface, const Address& addr) {}
    virtual void connect(const char* address, uint16_t port) {}
    virtual void disconnect() {}
    virtual bool isConnected() const { return false; }
    virtual void processPacket() {}
    virtual void sendPacket(const void* data, uint32_t size) {}

    virtual void writeConnectRequest(BitStream* stream) {}
    virtual bool readConnectRequest(BitStream* stream, const char** errorString) { return false; }
    virtual void writeConnectAccept(BitStream* stream) {}
    virtual bool readConnectAccept(BitStream* stream, const char** errorString) { return false; }

    virtual void onConnectTerminated(TerminationReason r, const char* reason) {}
    virtual void onConnectionEstablished() {}
    virtual void onConnectionTerminated(TerminationReason r, const char* error) {}
    virtual void onStartGhosting() {}
    virtual void onEndGhosting() {}

    bool isInitiator() const { return false; }
    bool isConnectionToServer() const { return false; }
    const char* getNetAddressString() const { return ""; }
    Address getNetAddress() const { return Address(); }
    virtual NetInterface* getInterface() { return nullptr; }

    // Additional RPCs
    virtual void c2sIntermissionDone() {}
    virtual void s2cStartIntermission(unsigned char a, unsigned char b, unsigned int c, unsigned int d, unsigned int e, unsigned int f) {}

    void setGhostFrom(bool from) {}
    void setGhostTo(bool to) {}
    void activateGhosting() {}
    void resetGhosting() {}
    void setScopeObject(NetObject* obj) {}
    void postNetEvent(NetEvent* event) {}

    void setIsAdaptive() {}
    void setTranslatesStrings() {}
    void setFixedRateParameters(U32 a, U32 b, U32 c, U32 d) {}
    void setSimulatedNetParams(float loss, int delay) {}

    int getConnectionState() const { return StateNotConnected; }

    // RPC stubs
    void rpcTest(U8 num) {}
    void rpcChat(S8 from, S8 to, StringPtr msg) {}
    void rpcPause(U8 pnum, bool on) {}
    void rpcMessage_s2c(S8 pnum, StringPtr msg, S8 priority, S8 type) {}
    void rpcSendNetVar_s2c(U16 netid, StringPtr s) {}
    void rpcKick_s2c(U8 pnum, StringPtr str) {}
    void rpcSendOptions_c2s(U8 pnum, ByteBufferPtr buf) {}
    void rpcSuicide_c2s(U8 pnum) {}
    void rpcRequestPOVchange_c2s(S32 pnum) {}
};

class GhostConnection : public NetConnection {
public:
    GhostConnection() {}
    virtual ~GhostConnection() {}
    void objectInScope(void* obj) {}
    S32 getGhostIndex(void* obj) { return -1; }
    void* resolveGhost(S32 idx) { return nullptr; }
};

class PlayerInfo;
class LocalPlayerInfo;

class NetInterface {
public:
    NetInterface() : mSocket(nullptr), mAllowConnections(false) {}
    NetInterface(const Address& addr) : mSocket(nullptr), mAllowConnections(false) {}
    virtual ~NetInterface() {}
    void setAllowsConnections(bool allow) { mAllowConnections = allow; }

    void* mSocket;
    bool mAllowConnections;

    virtual void disconnect(NetConnection* con, NetConnection::TerminationReason r, const char* reason) {}
    virtual void checkIncomingPackets() {}
    virtual void processConnections() {}

    // Stub implementations for called methods
    virtual void Kick(PlayerInfo*) {}
    virtual void SendPlayerOptions(int, LocalPlayerInfo&) {}
    virtual void SendChat(int, int, const char*) {}
    virtual void Pause(int, bool) {}
    virtual void RequestSuicide(int) {}
    virtual void SendNetVar(unsigned short, const char*) {}

    // Other methods if needed
};

// Utility functions
static U32 computeClientIdentityToken(const Address& addr, const Nonce& nonce) { return 0; }

#endif // TNL_STUB_H