#ifndef TNL_BITSTREAM_H
#define TNL_BITSTREAM_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <vector>

namespace TNL {

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
    
    void write(const void* data, size_t bits) {
        const uint8_t* src = static_cast<const uint8_t*>(data);
        size_t bytes = (bits + 7) >> 3;
        for (size_t i = 0; i < bytes; i++) {
            buffer.push_back(src[i]);
        }
    }
    
    void read(void* data, size_t bits) {
        uint8_t* dest = static_cast<uint8_t*>(data);
        size_t bytes = (bits + 7) >> 3;
        for (size_t i = 0; i < bytes && (currentBit >> 3) < buffer.size(); i++) {
            dest[i] = buffer[currentBit >> 3];
            currentBit += 8;
        }
    }
    
    void writeInt32(int32_t value, int bits) {
        write(&value, bits);
    }
    
    int32_t readInt32(int bits) {
        int32_t value = 0;
        read(&value, bits);
        return value;
    }
    
    void writeUInt32(uint32_t value, int bits) {
        write(&value, bits);
    }
    
    uint32_t readUInt32(int bits) {
        uint32_t value = 0;
        read(&value, bits);
        return value;
    }
    
    void writeFloat(float value) {
        write(&value, 32);
    }
    
    float readFloat() {
        float value = 0;
        read(&value, 32);
        return value;
    }
    
    void writeBool(bool value) {
        write(&value, 1);
    }
    
    bool readBool() {
        bool value = false;
        read(&value, 1);
        return value;
    }
    
    void writeString(const char* str) {
        size_t len = strlen(str) + 1;
        writeUInt32(len, 16);
        write(str, len * 8);
    }
    
    void readString(std::string& str) {
        uint32_t len = readUInt32(16);
        std::vector<char> buf(len);
        read(buf.data(), len * 8);
        str = buf.data();
    }
    
    void setBitPosition(size_t bit) { currentBit = bit; }
    size_t getBitPosition() const { return currentBit; }
    size_t getBitCount() const { return buffer.size() * 8; }
    size_t getByteCount() const { return buffer.size(); }
    
    void clear() { buffer.clear(); currentBit = 0; }
    bool isAtEnd() const { return (currentBit >> 3) >= buffer.size(); }
    
    uint8_t* getBuffer() { return buffer.data(); }
    const uint8_t* getBuffer() const { return buffer.data(); }
    
private:
    std::vector<uint8_t> buffer;
    size_t currentBit;
    bool ownBuffer;
};

} // namespace TNL

#endif // TNL_BITSTREAM_H
