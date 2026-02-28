#include "BitStreamSerializer.h"

namespace DoomLegacy::Network
{

BitStreamSerializer::BitStreamSerializer(TNL::BitStream* stream)
    : stream_(stream)
{
}

void BitStreamSerializer::write(const void* data, size_t bits)
{
    stream_->write(bits, data);
}

void BitStreamSerializer::write(uint8_t val)
{
    stream_->write(val);
}

void BitStreamSerializer::write(int8_t val)
{
    stream_->write(val);
}

void BitStreamSerializer::write(uint16_t val)
{
    stream_->write(val);
}

void BitStreamSerializer::write(int16_t val)
{
    stream_->write(val);
}

void BitStreamSerializer::write(uint32_t val)
{
    stream_->write(val);
}

void BitStreamSerializer::write(int32_t val)
{
    stream_->write(val);
}

void BitStreamSerializer::writeFloat(float val)
{
    stream_->write(val);
}

void BitStreamSerializer::writeString(const char* str)
{
    stream_->writeString(str);
}

void BitStreamSerializer::writeBool(bool val)
{
    stream_->writeFlag(val);
}

void BitStreamSerializer::read(void* data, size_t bits)
{
    stream_->read(bits, data);
}

uint8_t BitStreamSerializer::readUInt8()
{
    uint8_t v;
    stream_->read(&v);
    return v;
}

int8_t BitStreamSerializer::readInt8()
{
    int8_t v;
    stream_->read(&v);
    return v;
}

uint16_t BitStreamSerializer::readUInt16()
{
    uint16_t v;
    stream_->read(&v);
    return v;
}

int16_t BitStreamSerializer::readInt16()
{
    int16_t v;
    stream_->read(&v);
    return v;
}

uint32_t BitStreamSerializer::readUInt32()
{
    uint32_t v;
    stream_->read(&v);
    return v;
}

int32_t BitStreamSerializer::readInt32()
{
    int32_t v;
    stream_->read(&v);
    return v;
}

float BitStreamSerializer::readFloat()
{
    float v;
    stream_->read(&v);
    return v;
}

void BitStreamSerializer::readString(std::string& str)
{
    char temp[256];
    stream_->readString(temp);
    str = temp;
}

bool BitStreamSerializer::readBool()
{
    return stream_->readFlag();
}

void BitStreamSerializer::setBitPosition(size_t pos)
{
    stream_->setBitPosition(pos);
}

size_t BitStreamSerializer::getBitPosition() const
{
    return stream_->getBitPosition();
}

void BitStreamSerializer::clear()
{
    // BitStream doesn't have clear, perhaps reset position
    stream_->setBitPosition(0);
}

bool BitStreamSerializer::isAtEnd() const
{
    return stream_->isValid() && stream_->getBitPosition() >= stream_->getBitSpaceAllocated();
}

bool BitStreamSerializer::isWriting() const
{
    // BitStream doesn't distinguish, assume writing for now
    return true;
}

} // namespace DoomLegacy::Network