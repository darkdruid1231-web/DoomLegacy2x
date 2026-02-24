// Minimal TNL stub - no namespace conflicts with std
// Forward declarations only - no actual implementation

#ifndef TNL_STUB_H
#define TNL_STUB_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>

// Forward declarations
class BitStream;
class NetConnection;
class GhostConnection;

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
};

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

#endif // TNL_STUB_H
