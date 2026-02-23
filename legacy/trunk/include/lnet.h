// Stub header for lnet compatibility
#ifndef LNET_H
#define LNET_H 1

#include <cstdint>
#include <cstring>

namespace lnet
{

// Minimal BitStream stub to allow compilation
class BitStream
{
  private:
    uint8_t *buffer;
    size_t bufferSize;
    size_t bitPosition;
    size_t numBits;
    bool ownBuffer;

  public:
    BitStream(uint8_t *data = nullptr, bool own = false)
        : buffer(data), bufferSize(0), bitPosition(0), numBits(0), ownBuffer(own)
    {
    }

    ~BitStream()
    {
        if (ownBuffer && buffer)
        {
            delete[] buffer;
        }
    }

    void writeBits(const void *data, size_t bits)
    {
        // Stub implementation
    }

    void readBits(void *data, size_t bits)
    {
        // Stub implementation
        memset(data, 0, (bits + 7) / 8);
    }

    void write(uint8_t val)
    {
        writeBits(&val, 8);
    }
    void write(int8_t val)
    {
        writeBits(&val, 8);
    }
    void write(uint16_t val)
    {
        writeBits(&val, 16);
    }
    void write(int16_t val)
    {
        writeBits(&val, 16);
    }
    void write(uint32_t val)
    {
        writeBits(&val, 32);
    }
    void write(int32_t val)
    {
        writeBits(&val, 32);
    }

    uint8_t readUInt8()
    {
        uint8_t v;
        readBits(&v, 8);
        return v;
    }
    int8_t readInt8()
    {
        int8_t v;
        readBits(&v, 8);
        return v;
    }
    uint16_t readUInt16()
    {
        uint16_t v;
        readBits(&v, 16);
        return v;
    }
    int16_t readInt16()
    {
        int16_t v;
        readBits(&v, 16);
        return v;
    }
    uint32_t readUInt32()
    {
        uint32_t v;
        readBits(&v, 32);
        return v;
    }
    int32_t readInt32()
    {
        int32_t v;
        readBits(&v, 32);
        return v;
    }

    void writeFloat(float val)
    {
        writeBits(&val, 32);
    }
    float readFloat()
    {
        float v;
        readBits(&v, 32);
        return v;
    }

    // TNL-compatible bit-width int read/write.
    // Template avoids overload ambiguity for enum / long long / int arguments.
    template <typename T> void writeInt(T val, size_t bits)
    {
        int64_t v = static_cast<int64_t>(val);
        writeBits(&v, bits);
    }
    int32_t readInt(size_t bits)
    {
        int32_t v = 0;
        readBits(&v, bits);
        return v;
    }
    uint32_t readUInt(size_t bits)
    {
        uint32_t v = 0;
        readBits(&v, bits);
        return v;
    }

    void writeString(const char *str)
    {
        if (str)
        {
            while (*str)
            {
                write((uint8_t)*str);
                str++;
            }
        }
        write((uint8_t)0);
    }

    size_t getBitPosition() const
    {
        return bitPosition;
    }
    void setBitPosition(size_t pos)
    {
        bitPosition = pos;
    }
    size_t getNumBits() const
    {
        return numBits;
    }
    size_t getNumBytes() const
    {
        return (numBits + 7) / 8;
    }
    bool isAtEnd() const
    {
        return bitPosition >= numBits;
    }

    void setPlatformFlag(bool)
    {
    }
    bool getPlatformFlag() const
    {
        return false;
    }

    void clear()
    {
        bitPosition = 0;
    }
    void reset()
    {
        bitPosition = 0;
    }

    uint8_t *getBuffer()
    {
        return buffer;
    }
    const uint8_t *getBuffer() const
    {
        return buffer;
    }
};

} // namespace lnet

#endif
