#ifndef NET_BITSTREAM_SERIALIZER_H
#define NET_BITSTREAM_SERIALIZER_H

#include "core/ISerializer.h"
#include "tnl/tnlBitStream.h"

namespace DoomLegacy::Network
{

/**
 * @brief ISerializer implementation using TNL BitStream
 *
 * Allows existing TNL BitStream code to work with the new ISerializer interface.
 */
class BitStreamSerializer : public DoomLegacy::ISerializer
{
  public:
    explicit BitStreamSerializer(TNL::BitStream* stream);

    // Write methods
    void write(const void* data, size_t bits) override;
    void write(uint8_t val) override;
    void write(int8_t val) override;
    void write(uint16_t val) override;
    void write(int16_t val) override;
    void write(uint32_t val) override;
    void write(int32_t val) override;
    void writeFloat(float val) override;
    void writeString(const char* str) override;
    void writeBool(bool val) override;

    // Read methods
    void read(void* data, size_t bits) override;
    uint8_t readUInt8() override;
    int8_t readInt8() override;
    uint16_t readUInt16() override;
    int16_t readInt16() override;
    uint32_t readUInt32() override;
    int32_t readInt32() override;
    float readFloat() override;
    void readString(std::string& str) override;
    bool readBool() override;

    // Position
    void setBitPosition(size_t pos) override;
    size_t getBitPosition() const override;
    void clear() override;
    bool isAtEnd() const override;
    bool isWriting() const override;

  private:
    TNL::BitStream* stream_;
};

} // namespace DoomLegacy::Network

#endif // NET_BITSTREAM_SERIALIZER_H