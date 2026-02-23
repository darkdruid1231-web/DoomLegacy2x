// Comprehensive TNL stub header for build compatibility
// Provides no-op implementations of TNL types/macros so the engine compiles
// without the real TNL library (used on Windows / MSYS2 / cross-compile builds).
#ifndef TNL_STUB_H
#define TNL_STUB_H

/// Defined whenever the TNL stub headers are in use (i.e. non-Linux builds
/// and any build that lacks the real OpenTNL library).  Engine code can
/// guard TNL-specific implementation bodies with #ifndef TNL_STUB_BUILD.
#define TNL_STUB_BUILD 1

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>

// ============================================================
// Basic types
// ============================================================
typedef unsigned char   U8;
typedef signed char     S8;
typedef unsigned short  U16;
typedef signed short    S16;
typedef unsigned int    U32;
typedef signed int      S32;
typedef unsigned long long U64;
typedef signed long long S64;
namespace TNL {
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;
typedef float    F32;
typedef double   F64;

// BIT macro
#define BIT(n) (1 << (n))

// ============================================================
// Assert stubs
// ============================================================
#define TNLAssert(expr, msg)
#define TNLAssertMacro(expr, msg)
#define TNL_IMPLEMENT_NETCONNECTION(a,b,c)
#define LCONNECTION_RPC(name, params, args, delivery, dir, flags) void name params

inline void assertHandler(const char*, const char*, int, const char*) {}

// ============================================================
// NetClassRep
// ============================================================
class NetClassRep {
public:
    const char* className;
    U32 group;
    U32 bit;
    NetClassRep() : className(nullptr), group(0), bit(0) {}
};

// ============================================================
// BitStream stub
// ============================================================
class Address;
class BitStream {
public:
    BitStream() : currentBit(0), ownBuffer(true) {}
    explicit BitStream(size_t size) : buffer(size), currentBit(0), ownBuffer(true) {}
    BitStream(void* data, size_t size, bool own) : currentBit(0), ownBuffer(own) {
        if (data && size)
            buffer.assign(static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size);
    }
    ~BitStream() {}

    // Write methods
    void write(const void* data, size_t bits) {
        const uint8_t* src = static_cast<const uint8_t*>(data);
        size_t bytes = (bits + 7) >> 3;
        for (size_t i = 0; i < bytes; i++) buffer.push_back(src[i]);
    }
    void write(uint8_t  val) { write(&val, 8); }
    void write(int8_t   val) { write(&val, 8); }
    void write(uint16_t val) { write(&val, 16); }
    void write(int16_t  val) { write(&val, 16); }
    void write(uint32_t val) { write(&val, 32); }
    void write(int32_t  val) { write(&val, 32); }
    void writeFloat(float val)  { write(&val, 32); }
    void writeInt(int32_t  v, int bits) { write(&v, bits); }
    void writeUInt(uint32_t v, int bits) { write(&v, bits); }
    void writeUInt32(uint32_t val, int bits = 32) { write(&val, bits); }
    void writeBool(bool val) { write(&val, 1); }
    
    // Flag write (writes 1 bit, 1 for true, 0 for false)
    bool writeFlag(bool val) { writeBool(val); return true; }
    void writeString(const char* str) {
        size_t len = str ? strlen(str) + 1 : 0;
        writeUInt32((uint32_t)len, 16);
        if (len) write(str, len * 8);
    }

    // Read methods
    void read(void* data, size_t bits) {
        uint8_t* dest = static_cast<uint8_t*>(data);
        size_t bytes = (bits + 7) >> 3;
        for (size_t i = 0; i < bytes && (currentBit >> 3) < buffer.size(); i++) {
            dest[i] = buffer[currentBit >> 3];
            currentBit += 8;
        }
    }
    template<typename T> void read(T* dest) { read(dest, sizeof(T) * 8); }
    uint8_t  readUInt8()  { uint8_t  v; read(&v, 8);  return v; }
    int8_t   readInt8()   { int8_t   v; read(&v, 8);  return v; }
    uint16_t readUInt16() { uint16_t v; read(&v, 16); return v; }
    int16_t  readInt16()  { int16_t  v; read(&v, 16); return v; }
    uint32_t readUInt32() { uint32_t v; read(&v, 32); return v; }
    int32_t  readInt32()  { int32_t  v; read(&v, 32); return v; }
    float    readFloat()  { float    v; read(&v, 32); return v; }
    int32_t  readInt(int bits)  { int32_t  v; read(&v, bits); return v; }
    uint32_t readUInt(int bits) { uint32_t v; read(&v, bits); return v; }
    bool readBool() { bool v; read(&v, 1); return v; }
    
    // Flag read (reads 1 bit)
    bool readFlag() { return readBool(); }
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
        if (len > 0 && len < 65536) read(dest, len * 8);
    }
    // Overload with max buffer size (reads up to max-1 chars, null-terminates)
    void readString(char* dest, size_t maxLen) {
        uint32_t len = readUInt32();
        if (len > 0 && len < 65536 && len < maxLen) {
            read(dest, len * 8);
            dest[len] = '\0';
        } else {
            if (maxLen > 0) dest[0] = '\0';
        }
    }

    // Position helpers
    void   setBitPosition(size_t bit) { currentBit = bit; }
    size_t getBitPosition() const     { return currentBit; }
    void   setBytePosition(size_t p)  { currentBit = p * 8; }
    size_t getBytePosition() const    { return currentBit >> 3; }
    void   clear()  { buffer.clear(); currentBit = 0; }
    void   reset()  { currentBit = 0; }
    bool   isAtEnd() const { return (currentBit >> 3) >= buffer.size(); }
    void   sendto(void* socket, const Address& addr) {}
    uint8_t*       getBuffer()       { return buffer.data(); }
    const uint8_t* getBuffer() const { return buffer.data(); }

private:
    std::vector<uint8_t> buffer;
    size_t currentBit;
    bool   ownBuffer;
};

// BitStream method implementations

// ============================================================
// Forward declarations
// ============================================================
class NetInterface;
class NetObject;
class NetConnection;
class NetEvent;        // forward-declared so GhostConnection can use it
class Address;

// ============================================================
// Address / Nonce stubs
// ============================================================
class Address {
public:
    Address() : port(0) {}
    Address(const char* ip, uint16_t p) : ipStr(ip ? ip : ""), port(p) {}
    bool operator==(const Address& other) const {
        return ipStr == other.ipStr && port == other.port;
    }
    std::string toString() const {
        return ipStr + ":" + std::to_string(port);
    }
    U32 hash() const { return 0; }
    std::string ipStr;
    uint16_t port;
};

class Nonce {
public:
    Nonce() {}
    Nonce(const uint8_t*, size_t) {}
    void getRandom() {}
    void read(BitStream* stream) {}
    void write(BitStream* stream) const {}
    bool operator==(const Nonce& other) const { return true; }
    bool operator!=(const Nonce& other) const { return false; }
};

typedef BitStream PacketStream;

// ============================================================
// Utility functions
// ============================================================
inline U32 computeClientIdentityToken(const Address&, const Nonce&) { return 0; }

void checkIncomingPackets() {}
void processConnections() {}

// ============================================================
// Packet / connection constants
// ============================================================
const U32 FirstValidInfoPacketId  = 100;
const U32 NetClassGroupGame       = 0xFFFFFFFFu;
const U32 NetClassGroupGameMask   = 0xFFFFFFFFu;

// ============================================================
// TerminationReason enum (used in NetConnection callbacks)
// ============================================================
enum TerminationReason {
    ReasonTimedOut = 0,
    ReasonIdle,
    ReasonSelfDisconnect,
    ReasonRemoteDisconnect,
    ReasonDuplicateId,
    ReasonConnectsCancelled,
    ReasonConnectionError,
    ReasonMaxConnections,
    ReasonDisconnectPacket,
    ReasonMaxReasonCount
};

// ============================================================
// RPC direction / guarantee constants  (inside TNL namespace)
// ============================================================
enum RPCDirection {
    RPCToGhost       = 0,  ///< server → client (ghost-side)
    RPCToGhostParent = 1,  ///< client → server (ghost-parent side)
};

enum RPCGuarantee {
    RPCGuaranteedOrdered = 0,
    RPCGuaranteed        = 1,
    RPCUnguaranteed      = 2,
};

// ============================================================
// RPC pointer types used in RPC signatures
// ============================================================
typedef const char*             StringPtr;
typedef BitStream*   ByteBufferPtr;

// ============================================================
// NetEvent stub (target of postNetEvent / TNL_RPC_CONSTRUCT_NETEVENT)
// ============================================================
class NetEvent {
public:
    NetEvent() {}
    virtual ~NetEvent() {}
};

// ============================================================
// NetConnection stub
// ============================================================
class NetConnection {
public:
    NetConnection() : mInterface(nullptr), mState(0), mConnectId(0) {}
    virtual ~NetConnection() {}

    enum PacketType { PT_ServerPing = 0 };
    static const TerminationReason ReasonSelfDisconnect = TerminationReason::ReasonSelfDisconnect;

    virtual void connect(void*, const Address&) {}
    virtual void disconnect(NetConnection*, TerminationReason, const char*) {}
    virtual void update() {}
    virtual void sendPacket(BitStream*, uint8_t) {}
    virtual void onPacketReceived(uint8_t, BitStream*) {}
    virtual void onConnect() {}
    virtual void onDisconnect() {}
    virtual bool isInitiator() const { return false; }

    int          getState()      const { return mState; }
    int          getConnectionState() const { return mState; }
    void         setState(int s)       { mState = s; }
    NetInterface* getInterface()       { return mInterface; }
    void         setInterface(NetInterface* i) { mInterface = i; }
    uint32_t     getConnectId()  const { return mConnectId; }
    void         setConnectId(uint32_t id)    { mConnectId = id; }

private:
    NetInterface* mInterface;
    int           mState;
    uint32_t      mConnectId;
};

// ============================================================
// NetFlags helper (minimal BitSet-like type for mNetFlags)
// ============================================================
struct NetFlags {
    uint32_t bits;
    NetFlags() : bits(0) {}
    void set(uint32_t mask)        { bits |=  mask; }
    void clear(uint32_t mask)      { bits &= ~mask; }
    bool test(uint32_t mask) const { return (bits & mask) != 0; }
    bool any() const               { return bits != 0; }
};

// Flag constants used with mNetFlags (e.g. mNetFlags.set(Ghostable))
static const uint32_t Ghostable  = (1u << 0);
static const uint32_t ScopeLocal = (1u << 1);
static const uint32_t IsGhost    = (1u << 2);

// ============================================================
// NetObject stub
// ============================================================
class NetObject {
public:
    NetObject() : mNetInterface(nullptr), mGhostIndex(0) {}
    virtual ~NetObject() {}
    virtual void onGhostAdded()            {}
    virtual void onGhostRemoved()          {}
    virtual void onGhostUpdate(BitStream*) {}
    virtual void onGhostReceive(BitStream*){}
    virtual void packUpdate(BitStream*)    {}
    virtual bool unpackUpdate(BitStream*)  { return true; }

    // Dirty-mask tracking (no-op — no real ghosting infrastructure in stub)
    void setMaskBits(uint32_t /*mask*/)   {}
    void clearMaskBits(uint32_t /*mask*/) {}

    // isInitialUpdate: in real TNL, returns true when packUpdate is called for
    // the first time on a newly-ghosted object.  Stub always returns false.
    bool isInitialUpdate() const { return false; }

    NetInterface* getNetInterface()        { return mNetInterface; }
    void  setNetInterface(NetInterface* i) { mNetInterface = i; }
    int   getGhostIndex()  const           { return mGhostIndex; }
    void  setGhostIndex(int idx)           { mGhostIndex = idx; }

protected:
    // Engine code accesses mNetFlags directly (e.g. mNetFlags.set(Ghostable))
    NetFlags mNetFlags;

private:
    NetInterface* mNetInterface;
    int           mGhostIndex;
};

// ============================================================
// NetInterface stub
// ============================================================
class NetInterface {
public:
    NetInterface() : mAllowConnections(false), mSocket(nullptr) {}
    NetInterface(const Address&) : mAllowConnections(false), mSocket(nullptr) {}
    virtual ~NetInterface() {}
    virtual bool connect(const char*, uint16_t) { return false; }
    virtual void disconnect(NetConnection*, TerminationReason, const char*) {}
    virtual void update() {}
    virtual void send(NetConnection*, BitStream*, uint8_t) {}
    virtual void processPacket(NetConnection*, uint8_t, BitStream*) {}
    virtual void onConnect(NetConnection*) {}
    virtual void onDisconnect(NetConnection*) {}
    virtual void onConnectionRejected(NetConnection*, const char*) {}
    void setMaxConnections(int) {}
    void setAllowsConnections(bool allow) { mAllowConnections = allow; }
    int  getMaxConnections()     const { return 0; }
    int  getConnectionCount()    const { return 0; }
    NetConnection* getConnection(int)  { return nullptr; }
    bool mAllowConnections;
    void* mSocket;
};

// ============================================================
// GhostConnection stub
// ============================================================
class GhostConnection : public NetConnection {
public:
    GhostConnection() {}
    virtual ~GhostConnection() {}

    // Ghost management
    void addGhost(NetObject*)    {}
    void removeGhost(NetObject*) {}
    void updateGhost(NetObject*) {}
    NetObject* getGhost(int ghostIndex) const { return nullptr; }  // get ghost by index
    int getGhostIndex(const NetObject* /*obj*/) const { return -1; }  // get index of ghost
    virtual void onGhostAdded(NetObject*)   {}
    virtual void onGhostRemoved(NetObject*) {}
    virtual void onGhostUpdate(NetObject*)  {}

    // resolveGhost - resolve a ghost by index
    NetObject* resolveGhost(int /*ghostIndex*/) { return nullptr; }
    // isGhostAvailable(obj) — is obj already being ghosted to this connection?
    bool isGhostAvailable(NetObject* /*obj*/ = nullptr) const { return false; }
    void postNetEvent(NetEvent* /*e*/) {}

    // objectInScope - called to register objects in client's scope
    void objectInScope(NetObject* /*obj*/) {}

    // Connection lifecycle virtuals (declared in n_connection.h via override)
    virtual void onConnectionEstablished() {}
    virtual void onStartGhosting()         {}
    virtual void onEndGhosting()           {}

    // Connect request/accept virtuals (LConnection overrides these)
    virtual void writeConnectRequest(BitStream*) {}
    virtual bool readConnectRequest(BitStream*, const char**) { return true; }
    virtual void writeConnectAccept(BitStream*) {}
    virtual bool readConnectAccept(BitStream*, const char**) { return true; }

    // Termination callbacks (LConnection overrides these)
    virtual void onConnectTerminated(TerminationReason, const char*) {}
    
};

} // namespace TNL

// Global utility functions
inline TNL::Address getNetAddress() { return TNL::Address(); }
inline void setGhostFrom(bool) {}
inline void setGhostTo(bool) {}
inline const char* getNetAddressString() { return "127.0.0.1"; }
inline void resetGhosting() {}
inline void setScopeObject(void*) {}
inline void activateGhosting() {}
inline bool isConnectionToServer() { return false; }

// ============================================================
// RPC / NetEvent macros  (outside namespace — they are text substitutions)
// ============================================================

// TNL_DECLARE_RPC: expand to an inline virtual no-op method.  This covers all
// RPCs that have no out-of-class implementation (e.g. those in n_connection.h).
// The 3 PlayerInfo RPCs that *do* have bodies in g_player.cpp are wrapped in
// #ifndef TNL_STUB_BUILD guards there, so the inline stub is used instead.
#define TNL_DECLARE_RPC(name, params) virtual void name params {}

// TNL_IMPLEMENT_RPC: out-of-class implementation body — stub as no-op.
// Variadic because the real macro takes 8 arguments.
#define TNL_IMPLEMENT_RPC(...)

// TNL_DECLARE_NET_EVENT: declare a NetEvent subclass — stub as no-op.
#define TNL_DECLARE_NET_EVENT(cls)

// TNL_RPC_CONSTRUCT_NETEVENT: builds a NetEvent* to pass to postNetEvent().
// Stub returns nullptr; postNetEvent() is a no-op so this is safe.
#define TNL_RPC_CONSTRUCT_NETEVENT(conn, rpc, args) (static_cast<TNL::NetEvent*>(nullptr))

// TNL object registration macros — used at file scope in engine .cpp files
// (e.g. TNL_IMPLEMENT_NETOBJECT(PlayerInfo), TNL_IMPLEMENT_CLASS(GameType))
// Real TNL uses these to register classes with the netobject manager; stub as no-ops.
#define TNL_IMPLEMENT_NETOBJECT(cls)
// TNL_IMPLEMENT_NETOBJECT_RPC: no-op in stub builds.  The RPCs are already
// defined inline via TNL_DECLARE_RPC; the .cpp bodies are guarded by
// #ifndef TNL_STUB_BUILD so this macro is never followed by a stray { }.
#define TNL_IMPLEMENT_NETOBJECT_RPC(cls, name, args, argNames, group, guarantee, direction, version)
#define TNL_IMPLEMENT_CLASS(cls)

#endif // TNL_STUB_H
