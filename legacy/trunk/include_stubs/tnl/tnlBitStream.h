#ifndef TNL_BITSTREAM_H
#define TNL_BITSTREAM_H

#include <cstdint>
#include <cstring>

// Stub implementation of TNL BitStream
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
        // Stub implementation - fill with zeros
        memset(data, 0, (bits + 7) / 8);
    }

    bool writeFlag(bool val)
    {
        writeBits(&val, 1);
        return val;
    }

    bool readFlag()
    {
        bool v;
        readBits(&v, 1);
        return v;
    }

    void write(U8 val)
    {
        writeBits(&val, 8);
    }
    void write(S8 val)
    {
        writeBits(&val, 8);
    }
    void write(U16 val)
    {
        writeBits(&val, 16);
    }
    void write(S16 val)
    {
        writeBits(&val, 16);
    }
    void write(U32 val)
    {
        writeBits(&val, 32);
    }
    void write(S32 val)
    {
        writeBits(&val, 32);
    }

    U8 readUInt8()
    {
        U8 v;
        readBits(&v, 8);
        return v;
    }
    S8 readInt8()
    {
        S8 v;
        readBits(&v, 8);
        return v;
    }
    U16 readUInt16()
    {
        U16 v;
        readBits(&v, 16);
        return v;
    }
    S16 readInt16()
    {
        S16 v;
        readBits(&v, 16);
        return v;
    }
    U32 readUInt32()
    {
        U32 v;
        readBits(&v, 32);
        return v;
    }
    S32 readInt32()
    {
        S32 v;
        readBits(&v, 32);
        return v;
    }

    void writeFloat(F32 val)
    {
        writeBits(&val, 32);
    }
    F32 readFloat()
    {
        F32 v;
        readBits(&v, 32);
        return v;
    }

    // TNL-compatible bit-width int read/write.
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
                write((U8)*str);
                str++;
            }
        }
        write((U8)0);
    }

    void readString(char *str, size_t maxLen)
    {
        // Stub - read nothing
        if (str && maxLen > 0)
            str[0] = '\0';
    }

    // Additional read methods for compatibility
    void read(void *data, size_t bits)
    {
        readBits(data, bits);
    }

    void read(U16 *val)
    {
        *val = readUInt16();
    }

    void readString(char *str)
    {
        readString(str, 256); // Default max len
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
    bool isAtEnd() const
    {
        return bitPosition >= numBits;
    }

    void clear()
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

#endif // TNL_BITSTREAM_H