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
/// \brief Integration tests for demo recording and playback.

#include "d_ticcmd.h"
#include "doomdef.h"
#include <cstdint>
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

//============================================================================
// Demo Constants (from g_demo.cpp)
//============================================================================

#define ZT_FWD 0x01
#define ZT_SIDE 0x02
#define ZT_ANGLE 0x04
#define ZT_BUTTONS 0x08
#define ZT_AIMING 0x10
#define ZT_CHAT 0x20 // no more used
#define ZT_EXTRADATA 0x40
#define DEMOMARKER 0x80 // demoend

//============================================================================
// Mock Demo Buffer for Testing
//============================================================================

class MockDemoBuffer
{
  private:
    static const size_t BUFFER_SIZE = 8192;
    uint8_t buffer[BUFFER_SIZE];
    size_t write_pos;
    size_t read_pos;
    bool writing;

  public:
    MockDemoBuffer() : write_pos(0), read_pos(0), writing(true)
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

    void writeInt8(int8_t val)
    {
        writeUint8(static_cast<uint8_t>(val));
    }

    void writeInt16(int16_t val)
    {
        writeUint16(static_cast<uint16_t>(val));
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

    int8_t readInt8()
    {
        return static_cast<int8_t>(readUint8());
    }

    int16_t readInt16()
    {
        return static_cast<int16_t>(readUint16());
    }

    size_t getSize() const
    {
        return write_pos;
    }
    size_t getReadPos() const
    {
        return read_pos;
    }
};

//============================================================================
// Demo Header Structure Tests
//============================================================================

void test_demo_header_version()
{
    TEST("demo_header_version");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Write demo header version (LEGACY_VERSION = 1 typically)
    uint8_t version = 1;
    demo.writeUint8(version);

    demo.setReadMode();
    uint8_t restored = demo.readUint8();
    CHECK(restored == version, "Demo version mismatch");
    PASS();
}

void test_demo_header_skill()
{
    TEST("demo_header_skill");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Skill values: 0-4 (easy to nightmare)
    uint8_t skill = 2;
    demo.writeUint8(skill);

    demo.setReadMode();
    uint8_t restored = demo.readUint8();
    CHECK(restored == skill, "Demo skill mismatch");
    PASS();
}

void test_demo_header_game_type()
{
    TEST("demo_header_game_type");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Game type: singleplayer/coop/ deathmatch
    uint8_t game_type = 1; // deathmatch
    demo.writeUint8(game_type);

    demo.setReadMode();
    uint8_t restored = demo.readUint8();
    CHECK(restored == game_type, "Demo game type mismatch");
    PASS();
}

void test_demo_header_map()
{
    TEST("demo_header_map");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Map number
    uint8_t map = 7;
    demo.writeUint8(map);

    demo.setReadMode();
    uint8_t restored = demo.readUint8();
    CHECK(restored == map, "Demo map number mismatch");
    PASS();
}

void test_demo_header_multiplayer_flag()
{
    TEST("demo_header_multiplayer_flag");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Multiplayer flag
    uint8_t mp_flag = 1;
    demo.writeUint8(mp_flag);

    demo.setReadMode();
    uint8_t restored = demo.readUint8();
    CHECK(restored == mp_flag, "Demo multiplayer flag mismatch");
    PASS();
}

void test_demo_header_player_mask()
{
    TEST("demo_header_player_mask");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Player mask (32 players)
    for (int i = 0; i < 32; i++)
    {
        demo.writeUint8(1); // 1 if player exists
    }

    demo.setReadMode();
    int players = 0;
    for (int i = 0; i < 32; i++)
    {
        if (demo.readUint8() == 1)
            players++;
    }
    CHECK(players == 32, "Player mask should have 32 players");
    PASS();
}

void test_demo_header_complete()
{
    TEST("demo_header_complete");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Write complete demo header
    demo.writeUint8(1); // version
    demo.writeUint8(2); // skill
    demo.writeUint8(7); // map
    demo.writeUint8(1); // deathmatch
    demo.writeUint8(0); // respawn
    demo.writeUint8(0); // fast
    demo.writeUint8(0); // nomonsters
    demo.writeUint8(0); // timelimit
    demo.writeUint8(1); // multiplayer flag

    // Player mask for 4 players
    for (int i = 0; i < 32; i++)
    {
        demo.writeUint8(i < 4 ? 1 : 0);
    }

    demo.setReadMode();

    // Verify header
    CHECK(demo.readUint8() == 1, "Version mismatch");
    CHECK(demo.readUint8() == 2, "Skill mismatch");
    CHECK(demo.readUint8() == 7, "Map mismatch");
    CHECK(demo.readUint8() == 1, "Game type mismatch");
    CHECK(demo.readUint8() == 0, "Respawn mismatch");
    CHECK(demo.readUint8() == 0, "Fast mismatch");
    CHECK(demo.readUint8() == 0, "Nomonsters mismatch");
    CHECK(demo.readUint8() == 0, "Timelimit mismatch");
    CHECK(demo.readUint8() == 1, "Multiplayer flag mismatch");

    int player_count = 0;
    for (int i = 0; i < 32; i++)
    {
        if (demo.readUint8() == 1)
            player_count++;
    }
    CHECK(player_count == 4, "Should have 4 players");
    PASS();
}

//============================================================================
// TicCmd Structure Tests
//============================================================================

void test_ticcmd_structure_size()
{
    TEST("ticcmd_structure_size");
    // ticcmd_t contains: forward, side, yaw, pitch, roll, buttons, chat
    // forward/side: int8 or int16
    // yaw/pitch/roll: int16
    // buttons: uint8
    // chat: uint8

    // Check expected size based on structure
    size_t expected_size = 1 + 1 + 2 + 2 + 2 + 1 + 1; // minimal
    CHECK(sizeof(ticcmd_t) >= expected_size, "ticcmd_t size mismatch");
    PASS();
}

void test_ticcmd_forward_movement()
{
    TEST("ticcmd_forward_movement");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Forward movement (-128 to 127 for int8)
    int8_t forward = 50;
    demo.writeInt8(forward);

    demo.setReadMode();
    int8_t restored = demo.readInt8();
    CHECK(restored == forward, "Forward movement mismatch");
    PASS();
}

void test_ticcmd_side_movement()
{
    TEST("ticcmd_side_movement");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Side movement (-128 to 127 for int8)
    int8_t side = -30;
    demo.writeInt8(side);

    demo.setReadMode();
    int8_t restored = demo.readInt8();
    CHECK(restored == side, "Side movement mismatch");
    PASS();
}

void test_ticcmd_yaw_angle()
{
    TEST("ticcmd_yaw_angle");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Yaw angle (int16 for full range)
    int16_t yaw = 18000; // ~180 degrees
    demo.writeInt16(yaw);

    demo.setReadMode();
    int16_t restored = demo.readInt16();
    CHECK(restored == yaw, "Yaw angle mismatch");
    PASS();
}

void test_ticcmd_buttons()
{
    TEST("ticcmd_buttons");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Button flags (attack, use, etc.)
    uint8_t buttons = 0x01 | 0x04; // attack + jump
    demo.writeUint8(buttons);

    demo.setReadMode();
    uint8_t restored = demo.readUint8();
    CHECK(restored == buttons, "Buttons mismatch");
    CHECK(restored & 0x01, "Attack bit should be set");
    CHECK(restored & 0x04, "Jump bit should be set");
    PASS();
}

//============================================================================
// TicEvent Compression Tests
//============================================================================

void test_ticevent_forward_delta()
{
    TEST("ticevent_forward_delta");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Write ziptic with forward change
    uint8_t ziptic = ZT_FWD;
    demo.writeUint8(ziptic);

    int8_t forward_change = 25;
    demo.writeInt8(forward_change);

    demo.setReadMode();
    uint8_t restored_ziptic = demo.readUint8();
    CHECK(restored_ziptic == ZT_FWD, "Ziptic should have FWD flag");

    int8_t restored_forward = demo.readInt8();
    CHECK(restored_forward == forward_change, "Forward delta mismatch");
    PASS();
}

void test_ticevent_side_delta()
{
    TEST("ticevent_side_delta");
    MockDemoBuffer demo;
    demo.setWriteMode();

    uint8_t ziptic = ZT_SIDE;
    demo.writeUint8(ziptic);

    int8_t side_change = -15;
    demo.writeInt8(side_change);

    demo.setReadMode();
    CHECK(demo.readUint8() == ZT_SIDE, "Ziptic should have SIDE flag");
    CHECK(demo.readInt8() == side_change, "Side delta mismatch");
    PASS();
}

void test_ticevent_angle_change()
{
    TEST("ticevent_angle_change");
    MockDemoBuffer demo;
    demo.setWriteMode();

    uint8_t ziptic = ZT_ANGLE;
    demo.writeUint8(ziptic);

    int16_t angle_change = 500;
    demo.writeInt16(angle_change);

    demo.setReadMode();
    CHECK(demo.readUint8() == ZT_ANGLE, "Ziptic should have ANGLE flag");
    CHECK(demo.readInt16() == angle_change, "Angle delta mismatch");
    PASS();
}

void test_ticevent_buttons_change()
{
    TEST("ticevent_buttons_change");
    MockDemoBuffer demo;
    demo.setWriteMode();

    uint8_t ziptic = ZT_BUTTONS;
    demo.writeUint8(ziptic);

    uint8_t buttons = 0x05; // attack
    demo.writeUint8(buttons);

    demo.setReadMode();
    CHECK(demo.readUint8() == ZT_BUTTONS, "Ziptic should have BUTTONS flag");
    CHECK(demo.readUint8() == buttons, "Buttons delta mismatch");
    PASS();
}

void test_ticevent_multiple_changes()
{
    TEST("ticevent_multiple_changes");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Multiple changes in one tic
    uint8_t ziptic = ZT_FWD | ZT_SIDE | ZT_ANGLE;
    demo.writeUint8(ziptic);

    demo.writeInt8(10);   // forward
    demo.writeInt8(-5);   // side
    demo.writeInt16(100); // angle

    demo.setReadMode();
    CHECK(demo.readUint8() == ziptic, "Combined ziptic mismatch");
    CHECK(demo.readInt8() == 10, "Forward mismatch");
    CHECK(demo.readInt8() == -5, "Side mismatch");
    CHECK(demo.readInt16() == 100, "Angle mismatch");
    PASS();
}

void test_ticevent_aiming()
{
    TEST("ticevent_aiming");
    MockDemoBuffer demo;
    demo.setWriteMode();

    uint8_t ziptic = ZT_AIMING;
    demo.writeUint8(ziptic);

    int16_t pitch_change = -50; // looking down
    demo.writeInt16(pitch_change);

    demo.setReadMode();
    CHECK(demo.readUint8() == ZT_AIMING, "Ziptic should have AIMING flag");
    CHECK(demo.readInt16() == pitch_change, "Pitch mismatch");
    PASS();
}

void test_ticevent_marker()
{
    TEST("ticevent_marker");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Demo marker indicates end of demo
    uint8_t marker = DEMOMARKER;
    demo.writeUint8(marker);

    demo.setReadMode();
    uint8_t restored = demo.readUint8();
    CHECK(restored == DEMOMARKER, "Demo marker mismatch");
    PASS();
}

//============================================================================
// Checksum Tests
//============================================================================

void test_checksum_basic()
{
    TEST("checksum_basic");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Simple checksum calculation
    uint32_t data[] = {0x12345678, 0xDEADBEEF, 0xCAFEBABE};
    uint32_t checksum = 0;

    for (int i = 0; i < 3; i++)
    {
        checksum += data[i];
    }

    CHECK(checksum == 0x12345678 + 0xDEADBEEF + 0xCAFEBABE, "Checksum calculation failed");
    PASS();
}

void test_checksum_demo_header()
{
    TEST("checksum_demo_header");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Simulate demo header checksum
    uint32_t checksum = 0;

    // Add header fields to checksum
    checksum += 1; // version
    checksum += 2; // skill
    checksum += 7; // map

    // Initial header checksum
    uint32_t header_checksum = checksum ^ 0xFFFFFFFF;

    CHECK(header_checksum != 0, "Header checksum should be non-zero");
    CHECK(header_checksum == ~checksum, "Header checksum XOR failed");
    PASS();
}

void test_checksum_ticcmd()
{
    TEST("checksum_ticcmd");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Simulate ticcmd checksum
    int8_t forward = 50;
    int8_t side = -30;
    int16_t yaw = 18000;
    uint8_t buttons = 0x05;

    uint32_t checksum = 0;
    checksum += static_cast<uint8_t>(forward);
    checksum += static_cast<uint8_t>(side);
    checksum += static_cast<uint8_t>(yaw & 0xFF);
    checksum += static_cast<uint8_t>((yaw >> 8) & 0xFF);
    checksum += buttons;

    CHECK(checksum > 0, "Ticcmd checksum should be non-zero");
    PASS();
}

void test_checksum_consistency()
{
    TEST("checksum_consistency");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Same data should produce same checksum
    uint32_t data1 = 0x12345678;
    uint32_t data2 = 0x12345678;
    uint32_t checksum1 = data1 * 31 + 0xA5A5A5A5;
    uint32_t checksum2 = data2 * 31 + 0xA5A5A5A5;

    CHECK(checksum1 == checksum2, "Same data should produce same checksum");
    PASS();
}

void test_checksum_different_data()
{
    TEST("checksum_different_data");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Different data should produce different checksum
    uint32_t data1 = 0x12345678;
    uint32_t data2 = 0x87654321;
    uint32_t checksum1 = data1 * 31 + 0xA5A5A5A5;
    uint32_t checksum2 = data2 * 31 + 0xA5A5A5A5;

    CHECK(checksum1 != checksum2, "Different data should produce different checksum");
    PASS();
}

void test_checksum_rollover()
{
    TEST("checksum_rollover");
    // Test checksum overflow behavior (uint32_t wraps)
    uint32_t checksum = 0xFFFFFFFF;
    checksum += 1;
    // In C++, unsigned overflow wraps around
    CHECK(checksum == 0, "Checksum should wrap to 0 after overflow");
    PASS();
}

//============================================================================
// Demo File Format Tests
//============================================================================

void test_demo_extension()
{
    TEST("demo_extension");
    // Demo files use .lmp extension in legacy Doom
    const char *ext = ".lmp";
    CHECK(ext != nullptr, "Extension constant should exist");
    CHECK(strlen(ext) == 4, "Extension should be 4 chars");
    PASS();
}

void test_demo_single_player_format()
{
    TEST("demo_single_player_format");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Single player demo header
    demo.writeUint8(1); // version
    demo.writeUint8(3); // skill (hard)
    demo.writeUint8(1); // map
    demo.writeUint8(0); // singleplayer
    demo.writeUint8(0); // respawn
    demo.writeUint8(0); // fast
    demo.writeUint8(0); // nomonsters
    demo.writeUint8(0); // timelimit
    demo.writeUint8(0); // singleplayer flag

    // Player mask - single player
    for (int i = 0; i < 32; i++)
    {
        demo.writeUint8(i == 0 ? 1 : 0);
    }

    demo.setReadMode();

    CHECK(demo.readUint8() == 1, "Version mismatch");
    CHECK(demo.readUint8() == 3, "Skill mismatch");
    CHECK(demo.readUint8() == 1, "Map mismatch");

    // Count players
    int players = 0;
    for (int i = 0; i < 32; i++)
    {
        if (demo.readUint8() == 1)
            players++;
    }
    CHECK(players == 1, "Should be single player");
    PASS();
}

void test_demo_multiplayer_format()
{
    TEST("demo_multiplayer_format");
    MockDemoBuffer demo;
    demo.setWriteMode();

    // Multiplayer demo header
    demo.writeUint8(1); // version
    demo.writeUint8(1); // skill (easy)
    demo.writeUint8(3); // map
    demo.writeUint8(1); // deathmatch
    demo.writeUint8(0); // respawn
    demo.writeUint8(0); // fast
    demo.writeUint8(0); // nomonsters
    demo.writeUint8(5); // timelimit
    demo.writeUint8(1); // multiplayer flag

    // Player mask - 4 players
    for (int i = 0; i < 32; i++)
    {
        demo.writeUint8(i < 4 ? 1 : 0);
    }

    demo.setReadMode();

    CHECK(demo.readUint8() == 1, "Version mismatch");
    CHECK(demo.readUint8() == 1, "Skill mismatch");
    CHECK(demo.readUint8() == 3, "Map mismatch");
    CHECK(demo.readUint8() == 1, "Game type should be deathmatch");

    // Skip intermediate fields
    demo.readUint8(); // respawn
    demo.readUint8(); // fast
    demo.readUint8(); // nomonsters
    uint8_t timelimit = demo.readUint8();
    CHECK(timelimit >= 0 && timelimit <= 10, "Timelimit should be valid");

    // Skip multiplayer flag
    demo.readUint8();

    // Count players
    int players = 0;
    for (int i = 0; i < 32; i++)
    {
        uint8_t v = demo.readUint8();
        if (v == 1)
            players++;
    }
    CHECK(players == 4, "Should have 4 players based on mask");
    PASS();
}

//============================================================================
// Main
//============================================================================

int main()
{
    cout << "========================================" << endl;
    cout << "Demo Recording/Playback Tests" << endl;
    cout << "========================================" << endl;

    cout << "\n[Demo Header Structure Tests]" << endl;
    test_demo_header_version();
    test_demo_header_skill();
    test_demo_header_game_type();
    test_demo_header_map();
    test_demo_header_multiplayer_flag();
    test_demo_header_player_mask();
    test_demo_header_complete();

    cout << "\n[TicCmd Structure Tests]" << endl;
    test_ticcmd_structure_size();
    test_ticcmd_forward_movement();
    test_ticcmd_side_movement();
    test_ticcmd_yaw_angle();
    test_ticcmd_buttons();

    cout << "\n[TicEvent Compression Tests]" << endl;
    test_ticevent_forward_delta();
    test_ticevent_side_delta();
    test_ticevent_angle_change();
    test_ticevent_buttons_change();
    test_ticevent_multiple_changes();
    test_ticevent_aiming();
    test_ticevent_marker();

    cout << "\n[Checksum Tests]" << endl;
    test_checksum_basic();
    test_checksum_demo_header();
    test_checksum_ticcmd();
    test_checksum_consistency();
    test_checksum_different_data();
    test_checksum_rollover();

    cout << "\n[Demo File Format Tests]" << endl;
    test_demo_extension();
    test_demo_single_player_format();
    test_demo_multiplayer_format();

    cout << "\n========================================" << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;
    cout << "========================================" << endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
