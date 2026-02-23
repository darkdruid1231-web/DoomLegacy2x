#ifndef SERIALIZER_INTERFACE_H
#define SERIALIZER_INTERFACE_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

/**
 * @file ISerializer.h
 * @brief Serializer interface for data serialization/deserialization
 * 
 * This abstraction allows core utility code to serialize/deserialize data
 * without depending on specific networking implementations (TNL, ENet, etc.)
 * 
 * Following SOLID principles:
 * - Interface Segregation: Small, focused interface
 * - Dependency Inversion: Core code depends on abstraction, not implementation
 */

namespace DoomLegacy {

/// Message type for network transmission
enum class MessageType {
    Reliable,      /// Guaranteed delivery, ordered
    Unreliable,   /// Best effort, may be lost
    Ordered        /// Ordered delivery, may have gaps
};

/**
 * @brief Interface for serialization operations
 * 
 * This allows core code (like fixed_t, command system) to work with
 * serialization without knowing about networking specifics.
 */
class ISerializer {
public:
    virtual ~ISerializer() = default;
    
    // Write methods
    virtual void write(const void* data, size_t bits) = 0;
    virtual void write(uint8_t val) = 0;
    virtual void write(int8_t val) = 0;
    virtual void write(uint16_t val) = 0;
    virtual void write(int16_t val) = 0;
    virtual void write(uint32_t val) = 0;
    virtual void write(int32_t val) = 0;
    virtual void writeFloat(float val) = 0;
    virtual void writeString(const char* str) = 0;
    virtual void writeBool(bool val) = 0;
    
    // Read methods
    virtual void read(void* data, size_t bits) = 0;
    virtual uint8_t readUInt8() = 0;
    virtual int8_t readInt8() = 0;
    virtual uint16_t readUInt16() = 0;
    virtual int16_t readInt16() = 0;
    virtual uint32_t readUInt32() = 0;
    virtual int32_t readInt32() = 0;
    virtual float readFloat() = 0;
    virtual void readString(std::string& str) = 0;
    virtual bool readBool() = 0;
    
    // Position
    virtual void setBitPosition(size_t pos) = 0;
    virtual size_t getBitPosition() const = 0;
    virtual void clear() = 0;
    virtual bool isAtEnd() const = 0;

    /// Returns true if this serializer is in write mode (packing),
    /// false if it is in read mode (unpacking).
    /// Serialization methods that handle both directions (e.g. Actor::serialize)
    /// use this to branch between read and write operations.
    virtual bool isWriting() const = 0;
};

/**
 * @brief Simple in-memory serializer implementation for testing
 * 
 * This can be used by tests without any networking code.
 */
class InMemorySerializer : public ISerializer {
public:
    /// Default constructor: write/pack mode.
    InMemorySerializer() : buffer(), bitPos(0), writing_(true) {}
    /// Pre-allocate buffer; write mode by default.
    explicit InMemorySerializer(size_t reserve) : buffer(), bitPos(0), writing_(true) { buffer.reserve(reserve); }
    /// Explicit direction constructor.
    static InMemorySerializer forReading() { InMemorySerializer s; s.writing_ = false; return s; }
    
    // Write methods
    void write(const void* data, size_t bits) override {
        const uint8_t* src = static_cast<const uint8_t*>(data);
        size_t bytes = (bits + 7) >> 3;
        for (size_t i = 0; i < bytes; i++) {
            buffer.push_back(src[i]);
        }
    }
    void write(uint8_t val) override { write(&val, 8); }
    void write(int8_t val) override { write(&val, 8); }
    void write(uint16_t val) override { write(&val, 16); }
    void write(int16_t val) override { write(&val, 16); }
    void write(uint32_t val) override { write(&val, 32); }
    void write(uint32_t val, size_t bits) { write(&val, bits); }
    void write(int32_t val) override { write(&val, 32); }
    void writeFloat(float val) override { write(&val, 32); }
    void writeString(const char* str) override {
        if (str) {
            size_t len = strlen(str) + 1;
            write(static_cast<uint32_t>(len), 16);
            write(str, len * 8);
        } else {
            write(static_cast<uint32_t>(0), 16);
        }
    }
    void writeBool(bool val) override { write(&val, 1); }
    
    // Read methods
    void read(void* data, size_t bits) override {
        uint8_t* dest = static_cast<uint8_t*>(data);
        size_t bytes = (bits + 7) >> 3;
        for (size_t i = 0; i < bytes && (bitPos >> 3) < buffer.size(); i++) {
            dest[i] = buffer[bitPos >> 3];
            bitPos += 8;
        }
    }
    uint8_t readUInt8() override { uint8_t v; read(&v, 8); return v; }
    int8_t readInt8() override { int8_t v; read(&v, 8); return v; }
    uint16_t readUInt16() override { uint16_t v; read(&v, 16); return v; }
    int16_t readInt16() override { int16_t v; read(&v, 16); return v; }
    uint32_t readUInt32() override { uint32_t v; read(&v, 32); return v; }
    int32_t readInt32() override { int32_t v; read(&v, 32); return v; }
    float readFloat() override { float v; read(&v, 32); return v; }
    void readString(std::string& str) override {
        uint32_t len = readUInt32();
        if (len > 0 && len < 65536) {
            std::vector<char> buf(len);
            read(buf.data(), len * 8);
            str = buf.data();
        } else {
            str.clear();
        }
    }
    bool readBool() override { bool v; read(&v, 1); return v; }
    
    // Position
    void setBitPosition(size_t pos) override { bitPos = pos; }
    size_t getBitPosition() const override { return bitPos; }
    void clear() override { buffer.clear(); bitPos = 0; }
    bool isAtEnd() const override { return (bitPos >> 3) >= buffer.size(); }
    bool isWriting() const override { return writing_; }

    // Access
    const std::vector<uint8_t>& getBuffer() const { return buffer; }
    std::vector<uint8_t>& getBuffer() { return buffer; }

    /// Helper: reset to the beginning of the buffer for reading
    void rewindForReading() { bitPos = 0; writing_ = false; }

private:
    std::vector<uint8_t> buffer;
    size_t bitPos;
    bool writing_;
};

} // namespace DoomLegacy

#endif // SERIALIZER_INTERFACE_H
