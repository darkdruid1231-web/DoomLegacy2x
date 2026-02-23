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
/// \brief Integration tests for data serialization patterns.

#include "doomtype.h"
#include "m_fixed.h"
#include "vect.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;
static string last_failure;

#define TEST(name)                                                                                 \
    do                                                                                             \
    {                                                                                              \
        tests_run++;                                                                               \
        last_failure = "";                                                                         \
        cout << "  " << name << " ... ";                                                           \
    } while (0)

#define PASS()                                                                                     \
    do                                                                                             \
    {                                                                                              \
        tests_passed++;                                                                            \
        cout << "PASS" << endl;                                                                    \
    } while (0)

#define FAIL(msg)                                                                                  \
    do                                                                                             \
    {                                                                                              \
        last_failure = msg;                                                                        \
        cout << "FAIL: " << msg << endl;                                                           \
        return;                                                                                    \
    } while (0)

#define CHECK(cond, msg)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
            FAIL(msg);                                                                             \
    } while (0)

#define CHECK_CLOSE(actual, expected, tol)                                                         \
    do                                                                                             \
    {                                                                                              \
        if (std::abs((actual) - (expected)) > (tol))                                               \
        {                                                                                          \
            FAIL("Value " + to_string(actual) + " not within " + to_string(tol) + " of " +         \
                 to_string(expected));                                                             \
        }                                                                                          \
    } while (0)

//============================================================================
// Basic Type Serialization Tests (Simulated)
//============================================================================

// Simulated archive buffer for testing serialization patterns
class MockArchive
{
  private:
    static const size_t BUFFER_SIZE = 8192;
    uint8_t buffer[BUFFER_SIZE];
    size_t write_pos;
    size_t read_pos;
    bool writing;

  public:
    MockArchive() : write_pos(0), read_pos(0), writing(true)
    {
    }

    void setWriteMode()
    {
        writing = true;
        read_pos = 0;
        write_pos = 0;
    }

    void setReadMode()
    {
        writing = false;
        read_pos = 0;
    }

    // Write operations
    void writeUint8(uint8_t val)
    {
        if (writing && write_pos < BUFFER_SIZE)
        {
            buffer[write_pos++] = val;
        }
    }

    void writeUint16(uint16_t val)
    {
        if (writing && write_pos + 2 <= BUFFER_SIZE)
        {
            buffer[write_pos++] = val & 0xFF;
            buffer[write_pos++] = (val >> 8) & 0xFF;
        }
    }

    void writeUint32(uint32_t val)
    {
        if (writing && write_pos + 4 <= BUFFER_SIZE)
        {
            buffer[write_pos++] = val & 0xFF;
            buffer[write_pos++] = (val >> 8) & 0xFF;
            buffer[write_pos++] = (val >> 16) & 0xFF;
            buffer[write_pos++] = (val >> 24) & 0xFF;
        }
    }

    void writeInt32(int32_t val)
    {
        writeUint32(static_cast<uint32_t>(val));
    }

    void writeFloat(float val)
    {
        union
        {
            float f;
            uint32_t i;
        } conv;
        conv.f = val;
        writeUint32(conv.i);
    }

    // Read operations
    uint8_t readUint8()
    {
        if (!writing && read_pos < BUFFER_SIZE)
        {
            return buffer[read_pos++];
        }
        return 0;
    }

    uint16_t readUint16()
    {
        if (!writing && read_pos + 2 <= BUFFER_SIZE)
        {
            uint16_t val = buffer[read_pos++];
            val |= static_cast<uint16_t>(buffer[read_pos++]) << 8;
            return val;
        }
        return 0;
    }

    uint32_t readUint32()
    {
        if (!writing && read_pos + 4 <= BUFFER_SIZE)
        {
            uint32_t val = buffer[read_pos++];
            val |= static_cast<uint32_t>(buffer[read_pos++]) << 8;
            val |= static_cast<uint32_t>(buffer[read_pos++]) << 16;
            val |= static_cast<uint32_t>(buffer[read_pos++]) << 24;
            return val;
        }
        return 0;
    }

    int32_t readInt32()
    {
        return static_cast<int32_t>(readUint32());
    }

    float readFloat()
    {
        union
        {
            float f;
            uint32_t i;
        } conv;
        conv.i = readUint32();
        return conv.f;
    }

    size_t getSize() const
    {
        return write_pos;
    }
};

//============================================================================
// Uint8 Serialization Tests
//============================================================================

void test_serialize_uint8_basic()
{
    TEST("serialize_uint8_basic");
    MockArchive a;
    a.setWriteMode();

    uint8_t original = 0xAB;
    a.writeUint8(original);

    a.setReadMode();
    uint8_t restored = a.readUint8();
    CHECK(restored == original, "Uint8 serialization failed");
    PASS();
}

void test_serialize_uint8_range()
{
    TEST("serialize_uint8_range");
    MockArchive a;
    a.setWriteMode();

    a.writeUint8(0);
    a.writeUint8(127);
    a.writeUint8(128);
    a.writeUint8(255);

    a.setReadMode();
    CHECK(a.readUint8() == 0, "Min value failed");
    CHECK(a.readUint8() == 127, "Positive max failed");
    CHECK(a.readUint8() == 128, "High value failed");
    CHECK(a.readUint8() == 255, "Max value failed");
    PASS();
}

//============================================================================
// Uint16 Serialization Tests
//============================================================================

void test_serialize_uint16_basic()
{
    TEST("serialize_uint16_basic");
    MockArchive a;
    a.setWriteMode();

    uint16_t original = 0xABCD;
    a.writeUint16(original);

    a.setReadMode();
    uint16_t restored = a.readUint16();
    CHECK(restored == original, "Uint16 serialization failed");
    PASS();
}

void test_serialize_uint16_endianness()
{
    TEST("serialize_uint16_endianness");
    MockArchive a;
    a.setWriteMode();

    // Write in little-endian
    a.writeUint16(0x1234);

    a.setReadMode();
    uint8_t low = a.readUint8();
    uint8_t high = a.readUint8();
    CHECK(low == 0x34, "Low byte should be 0x34");
    CHECK(high == 0x12, "High byte should be 0x12");
    PASS();
}

//============================================================================
// Uint32 Serialization Tests
//============================================================================

void test_serialize_uint32_basic()
{
    TEST("serialize_uint32_basic");
    MockArchive a;
    a.setWriteMode();

    uint32_t original = 0xDEADBEEF;
    a.writeUint32(original);

    a.setReadMode();
    uint32_t restored = a.readUint32();
    CHECK(restored == original, "Uint32 serialization failed");
    PASS();
}

void test_serialize_int32_negative()
{
    TEST("serialize_int32_negative");
    MockArchive a;
    a.setWriteMode();

    int32_t original = -12345;
    a.writeInt32(original);

    a.setReadMode();
    int32_t restored = a.readInt32();
    CHECK(restored == original, "Int32 negative serialization failed");
    PASS();
}

//============================================================================
// Float Serialization Tests
//============================================================================

void test_serialize_float_basic()
{
    TEST("serialize_float_basic");
    MockArchive a;
    a.setWriteMode();

    float original = 3.14159f;
    a.writeFloat(original);

    a.setReadMode();
    float restored = a.readFloat();
    CHECK_CLOSE(restored, original, 0.001f);
    PASS();
}

void test_serialize_float_zero()
{
    TEST("serialize_float_zero");
    MockArchive a;
    a.setWriteMode();

    float original = 0.0f;
    a.writeFloat(original);

    a.setReadMode();
    float restored = a.readFloat();
    CHECK(restored == 0.0f, "Float zero serialization failed");
    PASS();
}

void test_serialize_float_negative()
{
    TEST("serialize_float_negative");
    MockArchive a;
    a.setWriteMode();

    float original = -273.15f;
    a.writeFloat(original);

    a.setReadMode();
    float restored = a.readFloat();
    CHECK_CLOSE(restored, original, 0.01f);
    PASS();
}

//============================================================================
// Fixed_t Serialization Tests
//============================================================================

void test_serialize_fixed_t_basic()
{
    TEST("serialize_fixed_t_basic");
    MockArchive a;
    a.setWriteMode();

    fixed_t original(42.5f);
    a.writeFloat(original.Float());

    a.setReadMode();
    float restored = a.readFloat();
    CHECK_CLOSE(restored, 42.5f, 0.1f);
    PASS();
}

void test_serialize_fixed_t_small()
{
    TEST("serialize_fixed_t_small");
    MockArchive a;
    a.setWriteMode();

    fixed_t original(0.001f);
    a.writeFloat(original.Float());

    a.setReadMode();
    float restored = a.readFloat();
    CHECK(restored > 0, "Small fixed_t should serialize to positive value");
    CHECK(restored < 0.01f, "Small fixed_t should be close to zero");
    PASS();
}

void test_serialize_fixed_t_large()
{
    TEST("serialize_fixed_t_large");
    MockArchive a;
    a.setWriteMode();

    fixed_t original(1000.0f);
    a.writeFloat(original.Float());

    a.setReadMode();
    float restored = a.readFloat();
    CHECK_CLOSE(restored, 1000.0f, 1.0f);
    PASS();
}

//============================================================================
// Vector Serialization Tests
//============================================================================

void test_serialize_2d_vector()
{
    TEST("serialize_2d_vector");
    MockArchive a;
    a.setWriteMode();

    vec_t<fixed_t> original;
    original.x = fixed_t(10.5f);
    original.y = fixed_t(20.25f);

    // Serialize vector components
    a.writeFloat(original.x.Float());
    a.writeFloat(original.y.Float());

    a.setReadMode();
    float x = a.readFloat();
    float y = a.readFloat();

    CHECK_CLOSE(x, 10.5f, 0.1f);
    CHECK_CLOSE(y, 20.25f, 0.1f);
    PASS();
}

void test_serialize_3d_vector()
{
    TEST("serialize_3d_vector");
    MockArchive a;
    a.setWriteMode();

    vec_t<fixed_t> original;
    original.x = fixed_t(100.0f);
    original.y = fixed_t(200.0f);
    original.z = fixed_t(50.0f);

    a.writeFloat(original.x.Float());
    a.writeFloat(original.y.Float());
    a.writeFloat(original.z.Float());

    a.setReadMode();
    float x = a.readFloat();
    float y = a.readFloat();
    float z = a.readFloat();

    CHECK_CLOSE(x, 100.0f, 1.0f);
    CHECK_CLOSE(y, 200.0f, 1.0f);
    CHECK_CLOSE(z, 50.0f, 0.5f);
    PASS();
}

//============================================================================
// Archive Size Tests
//============================================================================

void test_archive_size_single_value()
{
    TEST("archive_size_single_value");
    MockArchive a;
    a.setWriteMode();
    a.writeUint32(42);
    CHECK(a.getSize() == 4, "Uint32 should take 4 bytes");
    PASS();
}

void test_archive_size_multiple_values()
{
    TEST("archive_size_multiple_values");
    MockArchive a;
    a.setWriteMode();
    a.writeUint8(1);
    a.writeUint16(2);
    a.writeUint32(3);
    a.writeFloat(4.0f);
    CHECK(a.getSize() == 1 + 2 + 4 + 4, "Multiple values should have correct total size");
    PASS();
}

//============================================================================
// Pointer Reference Tests
//============================================================================

void test_pointer_reference_pattern()
{
    TEST("pointer_reference_pattern");
    // Test that we can serialize object IDs and restore pointer relationships
    int objects[3] = {100, 200, 300};
    int *ptrs[3] = {&objects[0], &objects[1], &objects[2]};
    uint32_t ids[3] = {0, 1, 2};

    // Serialize pointer IDs (simulated)
    MockArchive a;
    a.setWriteMode();
    for (int i = 0; i < 3; i++)
    {
        a.writeUint32(ids[i]);
    }

    // Restore pointer relationships
    a.setReadMode();
    for (int i = 0; i < 3; i++)
    {
        uint32_t id = a.readUint32();
        CHECK(id == ids[i], "Pointer ID should be preserved");
    }
    PASS();
}

//============================================================================
// Marker Pattern Tests
//============================================================================

void test_marker_pattern()
{
    TEST("marker_pattern");
    MockArchive a;
    a.setWriteMode();

    a.writeUint32(0xBEEF);
    a.writeUint32(0x1234); // Marker
    a.writeUint32(0xFEED);

    a.setReadMode();
    CHECK(a.readUint32() == 0xBEEF, "First value mismatch");

    // Check for marker
    uint32_t marker = a.readUint32();
    CHECK(marker == 0x1234, "Marker should be found");

    CHECK(a.readUint32() == 0xFEED, "Third value mismatch");
    PASS();
}

//============================================================================
// Main
//============================================================================

int main()
{
    cout << "========================================" << endl;
    cout << "Save/Load Serialization Tests" << endl;
    cout << "========================================" << endl;

    cout << "\n[Uint8 Serialization]" << endl;
    test_serialize_uint8_basic();
    test_serialize_uint8_range();

    cout << "\n[Uint16 Serialization]" << endl;
    test_serialize_uint16_basic();
    test_serialize_uint16_endianness();

    cout << "\n[Uint32 Serialization]" << endl;
    test_serialize_uint32_basic();
    test_serialize_int32_negative();

    cout << "\n[Float Serialization]" << endl;
    test_serialize_float_basic();
    test_serialize_float_zero();
    test_serialize_float_negative();

    cout << "\n[Fixed_t Serialization]" << endl;
    test_serialize_fixed_t_basic();
    test_serialize_fixed_t_small();
    test_serialize_fixed_t_large();

    cout << "\n[Vector Serialization]" << endl;
    test_serialize_2d_vector();
    test_serialize_3d_vector();

    cout << "\n[Archive Size Tests]" << endl;
    test_archive_size_single_value();
    test_archive_size_multiple_values();

    cout << "\n[Pointer Reference Tests]" << endl;
    test_pointer_reference_pattern();

    cout << "\n[Marker Pattern Tests]" << endl;
    test_marker_pattern();

    cout << "\n========================================" << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;
    cout << "========================================" << endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
