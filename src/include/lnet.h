// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: lnet.h 247 2006-04-08 13:16:08Z jussip $
//
// Copyright (C) 2004-2006 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
//
// Description:
//   BitStream class for network packet serialization
//
//-----------------------------------------------------------------------------

#ifndef LNET_H
#define LNET_H 1

#include <cstdint>
#include <cstring>
#include <algorithm>

namespace lnet
{

// BitStream for reading and writing binary data with bit-level precision
class BitStream
{
  private:
    uint8_t *buffer;
    size_t bufferSize;
    size_t bitPosition;
    size_t numBits;
    bool ownBuffer;

  public:
    BitStream()
        : buffer(nullptr), bufferSize(0), bitPosition(0), numBits(0), ownBuffer(false)
    {
    }

    BitStream(uint8_t *data, size_t size, bool owner = false)
        : buffer(data), bufferSize(size * 8), bitPosition(0), numBits(size * 8), ownBuffer(owner)
    {
    }

    ~BitStream()
    {
        if (ownBuffer && buffer)
        {
            delete[] buffer;
        }
    }

    // Write raw bits from data
    void writeBits(const void *data, size_t bits)
    {
        if (!data || bits == 0)
            return;

        const uint8_t *src = static_cast<const uint8_t *>(data);

        // Expand buffer if needed
        while (bitPosition + bits > bufferSize)
        {
            expandBuffer();
        }

        // Bit-level copy
        size_t startBit = bitPosition;
        for (size_t i = 0; i < bits; i++)
        {
            size_t srcByte = i / 8;
            size_t srcBit = i % 8;
            size_t dstBit = (startBit + i) % 8;
            size_t dstByte = (startBit + i) / 8;

            bool bit = (src[srcByte] >> srcBit) & 1;
            if (bit)
            {
                buffer[dstByte] |= (1 << dstBit);
            }
            else
            {
                buffer[dstByte] &= ~(1 << dstBit);
            }
        }

        bitPosition += bits;
        numBits = std::max(numBits, bitPosition);
    }

    // Read raw bits into data
    void readBits(void *data, size_t bits)
    {
        if (!data || bits == 0)
            return;

        uint8_t *dst = static_cast<uint8_t *>(data);
        memset(dst, 0, (bits + 7) / 8);

        size_t startBit = bitPosition;
        for (size_t i = 0; i < bits; i++)
        {
            size_t dstByte = i / 8;
            size_t dstBit = i % 8;
            size_t srcBit = (startBit + i) % 8;
            size_t srcByte = (startBit + i) / 8;

            if (srcByte >= (numBits + 7) / 8)
                break; // Out of bounds

            bool bit = (buffer[srcByte] >> srcBit) & 1;
            if (bit)
            {
                dst[dstByte] |= (1 << dstBit);
            }
        }

        bitPosition += bits;
    }

    // Write a single byte
    void write(uint8_t val)
    {
        writeBits(&val, 8);
    }

    // Write a single signed byte
    void write(int8_t val)
    {
        writeBits(&val, 8);
    }

    // Write a 16-bit unsigned
    void write(uint16_t val)
    {
        writeBits(&val, 16);
    }

    // Write a 16-bit signed
    void write(int16_t val)
    {
        writeBits(&val, 16);
    }

    // Write a 32-bit unsigned
    void write(uint32_t val)
    {
        writeBits(&val, 32);
    }

    // Write a 32-bit signed
    void write(int32_t val)
    {
        writeBits(&val, 32);
    }

    // Write a 64-bit unsigned
    void write(uint64_t val)
    {
        writeBits(&val, 64);
    }

    // Write a 64-bit signed
    void write(int64_t val)
    {
        writeBits(&val, 64);
    }

    // Write a float
    void writeFloat(float val)
    {
        writeBits(&val, 32);
    }

    // Write a double
    void writeDouble(double val)
    {
        writeBits(&val, 64);
    }

    // Write raw bytes (for TNL compatibility)
    void write(int size, const uint8_t* data)
    {
        writeBits(data, size * 8);
    }

    // Read raw bytes
    void read(uint8_t* data, int size)
    {
        readBits(data, size * 8);
    }

    // Read a byte
    uint8_t readUInt8()
    {
        uint8_t v;
        readBits(&v, 8);
        return v;
    }

    // Read a signed byte
    int8_t readInt8()
    {
        int8_t v;
        readBits(&v, 8);
        return v;
    }

    // Read a 16-bit unsigned
    uint16_t readUInt16()
    {
        uint16_t v;
        readBits(&v, 16);
        return v;
    }

    // Read a 16-bit signed
    int16_t readInt16()
    {
        int16_t v;
        readBits(&v, 16);
        return v;
    }

    // Read a 32-bit unsigned
    uint32_t readUInt32()
    {
        uint32_t v;
        readBits(&v, 32);
        return v;
    }

    // Read a 32-bit signed
    int32_t readInt32()
    {
        int32_t v;
        readBits(&v, 32);
        return v;
    }

    // Read a 64-bit unsigned
    uint64_t readUInt64()
    {
        uint64_t v;
        readBits(&v, 64);
        return v;
    }

    // Read a 64-bit signed
    int64_t readInt64()
    {
        int64_t v;
        readBits(&v, 64);
        return v;
    }

    // Read a float
    float readFloat()
    {
        float v;
        readBits(&v, 32);
        return v;
    }

    // Read a double
    double readDouble()
    {
        double v;
        readBits(&v, 64);
        return v;
    }

    // Write integer with specified bit count
    template <typename T>
    void writeInt(T val, size_t bits)
    {
        writeBits(&val, bits);
    }

    // Read integer with specified bit count
    int32_t readInt(size_t bits)
    {
        int32_t v = 0;
        readBits(&v, bits);
        return v;
    }

    // Read unsigned integer with specified bit count
    uint32_t readUInt(size_t bits)
    {
        uint32_t v = 0;
        readBits(&v, bits);
        return v;
    }

    // Read into a type T
    template <typename T>
    void read(T *data)
    {
        readBits(data, sizeof(T) * 8);
    }

    // Read specified number of bits into type T
    template <typename T>
    void read(T *data, size_t bits)
    {
        readBits(data, bits);
    }

    // Read a null-terminated string
    void readString(char *str, size_t maxLen)
    {
        size_t i = 0;
        while (i < maxLen - 1)
        {
            uint8_t c = readUInt8();
            str[i++] = c;
            if (c == 0)
                break;
        }
        str[i] = 0;
    }

    // Read string with default max length
    void readString(char *str)
    {
        readString(str, 256);
    }

    // Write a null-terminated string
    void writeString(const char *str)
    {
        if (str)
        {
            while (*str)
            {
                write(static_cast<uint8_t>(*str));
                str++;
            }
        }
        write(static_cast<uint8_t>(0)); // null terminator
    }

    // Read a single bit as a boolean flag
    bool readFlag()
    {
        uint8_t v;
        readBits(&v, 1);
        return v != 0;
    }

    // Write a boolean as a single bit
    bool writeFlag(bool flag)
    {
        uint8_t v = flag ? 1 : 0;
        writeBits(&v, 1);
        return flag;
    }

    // Get current bit position
    size_t getBitPosition() const
    {
        return bitPosition;
    }

    // Set bit position
    void setBitPosition(size_t pos)
    {
        bitPosition = std::min(pos, numBits);
    }

    // Get total bits written
    size_t getNumBits() const
    {
        return numBits;
    }

    // Get total bytes (rounded up)
    size_t getNumBytes() const
    {
        return (numBits + 7) / 8;
    }

    // Check if at end
    bool isAtEnd() const
    {
        return bitPosition >= numBits;
    }

    // Check if there are more bits available
    bool hasBits() const
    {
        return bitPosition < numBits;
    }

    // Get remaining bits
    size_t getRemainingBits() const
    {
        return numBits > bitPosition ? numBits - bitPosition : 0;
    }

    // Clear the stream
    void clear()
    {
        bitPosition = 0;
        numBits = 0;
        if (buffer && ownBuffer)
        {
            memset(buffer, 0, bufferSize / 8);
        }
    }

    // Reset for reading
    void reset()
    {
        bitPosition = 0;
    }

    // Get buffer pointer
    uint8_t *getBuffer()
    {
        return buffer;
    }

    const uint8_t *getBuffer() const
    {
        return buffer;
    }

    // Get buffer size in bits
    size_t getBufferSize() const
    {
        return bufferSize;
    }

    // Get byte position
    size_t getBytePosition() const
    {
        return bitPosition / 8;
    }

    // Set buffer (does not own)
    void setBuffer(uint8_t *data, size_t size)
    {
        buffer = data;
        bufferSize = size * 8;
        numBits = size * 8;
        bitPosition = 0;
        ownBuffer = false;
    }

    // Allocate own buffer
    void allocateBuffer(size_t size)
    {
        if (ownBuffer && buffer)
            delete[] buffer;
        buffer = new uint8_t[size];
        bufferSize = size * 8;
        numBits = 0;
        bitPosition = 0;
        ownBuffer = true;
        memset(buffer, 0, size);
    }

    // Align to next byte boundary
    void alignBits()
    {
        if (bitPosition % 8 != 0)
        {
            bitPosition += 8 - (bitPosition % 8);
        }
    }

    // Align to next byte boundary when reading
    void alignToByte()
    {
        alignBits();
    }

    // Write bits directly (for internal use)
    void writeBitsDirect(const uint8_t *data, size_t bits)
    {
        while (bitPosition + bits > bufferSize)
        {
            expandBuffer();
        }

        size_t startBit = bitPosition;
        for (size_t i = 0; i < bits; i++)
        {
            size_t srcByte = i / 8;
            size_t srcBit = i % 8;
            size_t dstBit = (startBit + i) % 8;
            size_t dstByte = (startBit + i) / 8;

            bool bit = (data[srcByte] >> srcBit) & 1;
            if (bit)
            {
                buffer[dstByte] |= (1 << dstBit);
            }
            else
            {
                buffer[dstByte] &= ~(1 << dstBit);
            }
        }

        bitPosition += bits;
        numBits = std::max(numBits, bitPosition);
    }

  private:
    void expandBuffer()
    {
        size_t newSize = bufferSize / 8 + 256;
        uint8_t *newBuffer = new uint8_t[newSize];
        memset(newBuffer, 0, newSize);

        if (buffer)
        {
            size_t oldBytes = (bufferSize + 7) / 8;
            memcpy(newBuffer, buffer, std::min(oldBytes, newSize));
            if (ownBuffer)
                delete[] buffer;
        }

        buffer = newBuffer;
        bufferSize = newSize * 8;
        ownBuffer = true;
    }
};

} // namespace lnet

#endif
