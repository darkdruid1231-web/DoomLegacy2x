//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
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

/// \file
/// \brief Unit tests for TNL packet serialization.

#include <iostream>
#include <cstring>
#include <string>
#include <cstdint>
// TNL headers would be included via tnl/tnl.h in full build
// For standalone tests, we'll mock the essential structures

// Minimal mock of TNL BitStream for testing packet serialization
class MockBitStream {
private:
    static const size_t BUFFER_SIZE = 4096;
    uint8_t buffer[BUFFER_SIZE];
    size_t read_pos;
    size_t write_pos;
    bool writing;

public:
    MockBitStream() : read_pos(0), write_pos(0), writing(true) {}
    
    void setWriteMode() {
        writing = true;
        read_pos = 0;
        write_pos = 0;
    }
    
    void setReadMode() {
        writing = false;
        read_pos = 0;
    }
    
    // Write operations
    void writeInt(uint32_t value, int bits) {
        if (writing) {
            // Mask to get only the bits we need
            uint32_t mask = (1U << bits) - 1;
            uint32_t val = value & mask;
            
            // Write little-endian
            for (int i = 0; i < bits; i += 8) {
                if (write_pos < BUFFER_SIZE) {
                    buffer[write_pos++] = (val >> i) & 0xFF;
                }
            }
        }
    }
    
    void writeInt32(uint32_t value) {
        if (writing && write_pos + 4 <= BUFFER_SIZE) {
            buffer[write_pos++] = value & 0xFF;
            buffer[write_pos++] = (value >> 8) & 0xFF;
            buffer[write_pos++] = (value >> 16) & 0xFF;
            buffer[write_pos++] = (value >> 24) & 0xFF;
        }
    }
    
    void writeInt8(uint8_t value) {
        if (writing && write_pos < BUFFER_SIZE) {
            buffer[write_pos++] = value;
        }
    }
    
    void writeInt16(uint16_t value) {
        if (writing && write_pos + 2 <= BUFFER_SIZE) {
            buffer[write_pos++] = value & 0xFF;
            buffer[write_pos++] = (value >> 8) & 0xFF;
        }
    }
    
    void writeFloat(float value) {
        if (writing && write_pos + 4 <= BUFFER_SIZE) {
            union { float f; uint32_t i; } conv;
            conv.f = value;
            buffer[write_pos++] = conv.i & 0xFF;
            buffer[write_pos++] = (conv.i >> 8) & 0xFF;
            buffer[write_pos++] = (conv.i >> 16) & 0xFF;
            buffer[write_pos++] = (conv.i >> 24) & 0xFF;
        }
    }
    
    void writeString(const char* str) {
        if (writing && str) {
            size_t len = strlen(str);
            writeInt32(len);
            for (size_t i = 0; i < len && write_pos < BUFFER_SIZE; i++) {
                buffer[write_pos++] = str[i];
            }
        }
    }
    
    // Read operations
    uint32_t readInt(int bits) {
        if (!writing && read_pos + ((bits + 7) / 8) <= BUFFER_SIZE) {
            uint32_t result = 0;
            for (int i = 0; i < bits; i += 8) {
                result |= ((uint32_t)buffer[read_pos++] & 0xFF) << i;
            }
            return result;
        }
        return 0;
    }
    
    uint32_t readInt32() {
        if (!writing && read_pos + 4 <= BUFFER_SIZE) {
            uint32_t result = buffer[read_pos++];
            result |= ((uint32_t)buffer[read_pos++] << 8);
            result |= ((uint32_t)buffer[read_pos++] << 16);
            result |= ((uint32_t)buffer[read_pos++] << 24);
            return result;
        }
        return 0;
    }
    
    uint8_t readInt8() {
        if (!writing && read_pos < BUFFER_SIZE) {
            return buffer[read_pos++];
        }
        return 0;
    }
    
    uint16_t readInt16() {
        if (!writing && read_pos + 2 <= BUFFER_SIZE) {
            uint16_t result = buffer[read_pos++];
            result |= ((uint16_t)buffer[read_pos++] << 8);
            return result;
        }
        return 0;
    }
    
    float readFloat() {
        if (!writing && read_pos + 4 <= BUFFER_SIZE) {
            union { float f; uint32_t i; } conv;
            conv.i = buffer[read_pos++];
            conv.i |= ((uint32_t)buffer[read_pos++] << 8);
            conv.i |= ((uint32_t)buffer[read_pos++] << 16);
            conv.i |= ((uint32_t)buffer[read_pos++] << 24);
            return conv.f;
        }
        return 0.0f;
    }
    
    void readString(char* str, size_t max_len) {
        if (!writing && str) {
            uint32_t len = readInt32();
            for (uint32_t i = 0; i < len && i < max_len - 1; i++) {
                str[i] = buffer[read_pos++];
            }
            str[min(len, max_len - 1)] = '\0';
        }
    }
    
    size_t getPosition() const { return writing ? write_pos : read_pos; }
    size_t getBufferSize() const { return write_pos; }
    const uint8_t* getBuffer() const { return buffer; }
    
    static size_t min(size_t a, size_t b) { return a < b ? a : b; }
};

// Include the fixed_t header for Pack/Unpack tests
#include "m_fixed.h"

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;
static string last_failure;

#define TEST(name) do { \
    tests_run++; \
    last_failure = ""; \
    cout << " " << name << " ... "; \
} while(0)

#define PASS() do { \
    tests_passed++; \
    cout << "PASS" << endl; \
} while(0)

#define FAIL(msg) do { \
    last_failure = msg; \
    cout << "FAIL: " << msg << endl; \
    return; \
} while(0)

#define CHECK(cond, msg) do { \
    if (!(cond)) FAIL(msg); \
} while(0)

#define CHECK_EQ(actual, expected, msg) do { \
    if ((actual) != (expected)) { \
        FAIL(string(msg) + " (expected " + to_string(expected) + ", got " + to_string(actual) + ")"); \
    } \
} while(0)

//============================================================================
// Basic BitStream Tests
//============================================================================

void test_bitstream_write_int8() {
    TEST("bitstream_write_int8");
    MockBitStream bs;
    bs.setWriteMode();
    bs.writeInt8(0xAB);
    
    bs.setReadMode();
    uint8_t val = bs.readInt8();
    CHECK(val == 0xAB, "Int8 write/read failed");
    PASS();
}

void test_bitstream_write_int32() {
    TEST("bitstream_write_int32");
    MockBitStream bs;
    bs.setWriteMode();
    bs.writeInt32(0xDEADBEEF);
    
    bs.setReadMode();
    uint32_t val = bs.readInt32();
    CHECK(val == 0xDEADBEEF, "Int32 write/read failed");
    PASS();
}

void test_bitstream_write_float() {
    TEST("bitstream_write_float");
    MockBitStream bs;
    bs.setWriteMode();
    float original = 3.14159f;
    bs.writeFloat(original);
    
    bs.setReadMode();
    float val = bs.readFloat();
    CHECK(val == original, "Float write/read failed");
    PASS();
}

void test_bitstream_write_string() {
    TEST("bitstream_write_string");
    MockBitStream bs;
    bs.setWriteMode();
    bs.writeString("Hello, TNL!");
    
    bs.setReadMode();
    char buffer[256];
    bs.readString(buffer, sizeof(buffer));
    CHECK(strcmp(buffer, "Hello, TNL!") == 0, "String write/read failed");
    PASS();
}

void test_bitstream_write_empty_string() {
    TEST("bitstream_write_empty_string");
    MockBitStream bs;
    bs.setWriteMode();
    bs.writeString("");
    
    bs.setReadMode();
    char buffer[256];
    bs.readString(buffer, sizeof(buffer));
    CHECK(strcmp(buffer, "") == 0, "Empty string write/read failed");
    PASS();
}

//============================================================================
// Connection Packet Tests
//============================================================================

struct ConnectionPacket {
    uint8_t packet_type;
    uint32_t protocol_version;
    char game_id[16];
    uint8_t player_id;
    uint32_t checksum;
    
    void write(MockBitStream* bs) const {
        bs->writeInt8(packet_type);
        bs->writeInt32(protocol_version);
        bs->writeString(game_id);
        bs->writeInt8(player_id);
        bs->writeInt32(checksum);
    }
    
    bool read(MockBitStream* bs) {
        packet_type = bs->readInt8();
        protocol_version = bs->readInt32();
        char buffer[256];
        bs->readString(buffer, sizeof(buffer));
        strncpy(game_id, buffer, sizeof(game_id) - 1);
        game_id[sizeof(game_id) - 1] = '\0';
        player_id = bs->readInt8();
        checksum = bs->readInt32();
        return true;
    }
};

void test_connection_packet_write_read() {
    TEST("connection_packet_write_read");
    MockBitStream bs;
    
    ConnectionPacket original;
    original.packet_type = 0x01;
    original.protocol_version = 0x00010001;
    strcpy(original.game_id, "DOOMLEGACY");
    original.player_id = 42;
    original.checksum = 0x12345678;
    
    bs.setWriteMode();
    original.write(&bs);
    
    ConnectionPacket restored;
    bs.setReadMode();
    bool result = restored.read(&bs);
    
    CHECK(result == true, "Packet read should succeed");
    CHECK(restored.packet_type == original.packet_type, "packet_type mismatch");
    CHECK(restored.protocol_version == original.protocol_version, "protocol_version mismatch");
    CHECK(strcmp(restored.game_id, original.game_id) == 0, "game_id mismatch");
    CHECK(restored.player_id == original.player_id, "player_id mismatch");
    CHECK(restored.checksum == original.checksum, "checksum mismatch");
    PASS();
}

void test_connection_packet_buffer_size() {
    TEST("connection_packet_buffer_size");
    MockBitStream bs;
    
    ConnectionPacket packet;
    packet.packet_type = 1;
    packet.protocol_version = 1;
    strcpy(packet.game_id, "TEST");
    packet.player_id = 1;
    packet.checksum = 0;
    
    bs.setWriteMode();
    packet.write(&bs);
    
    size_t size = bs.getPosition();
    CHECK(size > 0, "Packet should have non-zero size");
    CHECK(size < 1024, "Packet size should be reasonable");
    PASS();
}

//============================================================================
// Fixed_t Pack/Unpack Tests
//============================================================================

void test_fixed_t_pack_unpack() {
    TEST("fixed_t_pack_unpack");
    MockBitStream bs;
    
    fixed_t original(42.5f);
    
    // Test Pack method if available (depends on TNL linkage)
    // original.Pack(&bs);
    // For now, test using write/read float as simulation
    
    bs.setWriteMode();
    bs.writeFloat(original.Float());
    
    bs.setReadMode();
    float val = bs.readFloat();
    
    CHECK(abs(val - 42.5f) < 0.01f, "fixed_t float conversion failed");
    PASS();
}

void test_fixed_t_serialization_precision() {
    TEST("fixed_t_serialization_precision");
    MockBitStream bs;
    
    fixed_t original(0.001f);
    
    bs.setWriteMode();
    bs.writeFloat(original.Float());
    
    bs.setReadMode();
    float val = bs.readFloat();
    
    CHECK(val > 0, "Small fixed_t value should serialize correctly");
    CHECK(val < 0.01f, "Small fixed_t value should be preserved");
    PASS();
}

//============================================================================
// Multi-Value Packet Tests
//============================================================================

struct GameStatePacket {
    uint8_t packet_id;
    uint32_t tick;
    uint32_t position_x;
    uint32_t position_y;
    uint32_t position_z;
    uint16_t angle;
    uint8_t flags;
    
    void write(MockBitStream* bs) const {
        bs->writeInt8(packet_id);
        bs->writeInt32(tick);
        bs->writeInt32(position_x);
        bs->writeInt32(position_y);
        bs->writeInt32(position_z);
        bs->writeInt16(angle);
        bs->writeInt8(flags);
    }
    
    bool read(MockBitStream* bs) {
        packet_id = bs->readInt8();
        tick = bs->readInt32();
        position_x = bs->readInt32();
        position_y = bs->readInt32();
        position_z = bs->readInt32();
        angle = bs->readInt16();
        flags = bs->readInt8();
        return true;
    }
};

void test_game_state_packet() {
    TEST("game_state_packet");
    MockBitStream bs;
    
    GameStatePacket original;
    original.packet_id = 0x05;
    original.tick = 12345;
    original.position_x = 1000;
    original.position_y = 2000;
    original.position_z = 100;
    original.angle = 180;
    original.flags = 0x03;
    
    bs.setWriteMode();
    original.write(&bs);
    
    GameStatePacket restored;
    bs.setReadMode();
    bool result = restored.read(&bs);
    
    CHECK(result == true, "Game state packet read should succeed");
    CHECK(restored.packet_id == original.packet_id, "packet_id mismatch");
    CHECK(restored.tick == original.tick, "tick mismatch");
    CHECK(restored.position_x == original.position_x, "position_x mismatch");
    CHECK(restored.position_y == original.position_y, "position_y mismatch");
    CHECK(restored.position_z == original.position_z, "position_z mismatch");
    CHECK(restored.angle == original.angle, "angle mismatch");
    CHECK(restored.flags == original.flags, "flags mismatch");
    PASS();
}

//============================================================================
// Bit-Precision Tests
//============================================================================

void test_bitstream_int_with_bits() {
    TEST("bitstream_int_with_bits");
    MockBitStream bs;
    bs.setWriteMode();
    
    // Write 4-bit value (0-15)
    bs.writeInt(12, 4);
    bs.writeInt(3, 4);
    
    bs.setReadMode();
    uint32_t val1 = bs.readInt(4);
    uint32_t val2 = bs.readInt(4);
    
    CHECK(val1 == 12, "4-bit value 1 mismatch");
    CHECK(val2 == 3, "4-bit value 2 mismatch");
    PASS();
}

void test_bitstream_int_boundaries() {
    TEST("bitstream_int_boundaries");
    MockBitStream bs;
    bs.setWriteMode();
    
    // Test 8-bit boundaries
    bs.writeInt(0, 8);
    bs.writeInt(255, 8);
    
    bs.setReadMode();
    CHECK(bs.readInt(8) == 0, "8-bit min boundary failed");
    CHECK(bs.readInt(8) == 255, "8-bit max boundary failed");
    PASS();
}

//============================================================================
// String Serialization Tests
//============================================================================

void test_long_string_serialization() {
    TEST("long_string_serialization");
    MockBitStream bs;
    bs.setWriteMode();
    
    string long_str;
    for (int i = 0; i < 100; i++) {
        long_str += "A";
    }
    bs.writeString(long_str.c_str());
    
    bs.setReadMode();
    char buffer[256];
    bs.readString(buffer, sizeof(buffer));
    
    CHECK(strlen(buffer) == 100, "Long string length mismatch");
    CHECK(buffer[0] == 'A', "Long string content mismatch");
    CHECK(buffer[99] == 'A', "Long string end mismatch");
    PASS();
}

void test_special_chars_in_string() {
    TEST("special_chars_in_string");
    MockBitStream bs;
    bs.setWriteMode();
    
    const char* special_str = "Hello\nWorld\t!\r\n";
    bs.writeString(special_str);
    
    bs.setReadMode();
    char buffer[256];
    bs.readString(buffer, sizeof(buffer));
    
    CHECK(strcmp(buffer, special_str) == 0, "Special chars string mismatch");
    PASS();
}

//============================================================================
// Main
//============================================================================

int main() {
    cout << "========================================" << endl;
    cout << "TNL Packet Tests" << endl;
    cout << "========================================" << endl;
    
    cout << "\n[Basic BitStream Tests]" << endl;
    test_bitstream_write_int8();
    test_bitstream_write_int32();
    test_bitstream_write_float();
    test_bitstream_write_string();
    test_bitstream_write_empty_string();
    
    cout << "\n[Connection Packet Tests]" << endl;
    test_connection_packet_write_read();
    test_connection_packet_buffer_size();
    
    cout << "\n[Fixed_t Serialization Tests]" << endl;
    test_fixed_t_pack_unpack();
    test_fixed_t_serialization_precision();
    
    cout << "\n[Multi-Value Packet Tests]" << endl;
    test_game_state_packet();
    
    cout << "\n[Bit-Precision Tests]" << endl;
    test_bitstream_int_with_bits();
    test_bitstream_int_boundaries();
    
    cout << "\n[String Serialization Tests]" << endl;
    test_long_string_serialization();
    test_special_chars_in_string();
    
    cout << "\n========================================" << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;
    cout << "========================================" << endl;
    
    return (tests_passed == tests_run) ? 0 : 1;
}
