//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Google Test tests for SoundEmitterComponent using MockSoundSystem.

#include "interfaces/i_sound.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// Local mock matching MockSoundSystem interface
class MockSoundSystemLocal : public ISoundSystem {
public:
    MOCK_METHOD(int, startSound, (Actor* mobj, int sfx_id, float vol), (override));
    MOCK_METHOD(void, stopSound, (Actor* mobj), (override));
    MOCK_METHOD(void, setListenerPosition, (float x, float y, float z), (override));
    MOCK_METHOD(bool, isChannelPlaying, (int channel), (const override));
    MOCK_METHOD(void, setChannelVolume, (int channel, float vol), (override));
    MOCK_METHOD(int, getActiveChannelCount, (), (const override));
    MOCK_METHOD(void, precacheSound, (int sfx_id), (override));
};

// Component that plays sounds at an actor's position
class SoundEmitterComponent {
public:
    explicit SoundEmitterComponent(ISoundSystem* sound, Actor* owner)
        : sound_(sound), owner_(owner), channel_(-1) {}

    void playSound(int sfx_id, float vol) {
        channel_ = sound_->startSound(owner_, sfx_id, vol);
    }

    void stopSound() {
        if (channel_ >= 0) {
            sound_->stopSound(owner_);
            channel_ = -1;
        }
    }

    void onOwnerDeath() {
        stopSound();
    }

    bool isPlaying() const {
        return channel_ >= 0 && sound_->isChannelPlaying(channel_);
    }

    int getChannel() const { return channel_; }

private:
    ISoundSystem* sound_;
    Actor* owner_;
    int channel_;
};

// Tests

TEST(SoundEmitter, playsSoundOnRequest)
{
    NiceMock<MockSoundSystemLocal> mock;

    ON_CALL(mock, startSound(nullptr, 5, 0.8f)).WillByDefault(Return(3));
    ON_CALL(mock, isChannelPlaying(3)).WillByDefault(Return(true));

    SoundEmitterComponent emitter(&mock, nullptr);
    emitter.playSound(5, 0.8f);

    EXPECT_TRUE(emitter.isPlaying());
    EXPECT_EQ(emitter.getChannel(), 3);
}

TEST(SoundEmitter, stopsOnDeath)
{
    NiceMock<MockSoundSystemLocal> mock;

    ON_CALL(mock, startSound(nullptr, 5, 0.5f)).WillByDefault(Return(2));

    SoundEmitterComponent emitter(&mock, nullptr);
    emitter.playSound(5, 0.5f);

    EXPECT_CALL(mock, stopSound(nullptr)).Times(1);
    emitter.onOwnerDeath();
}

TEST(SoundEmitter, silentWhenNoChannel)
{
    NiceMock<MockSoundSystemLocal> mock;

    ON_CALL(mock, startSound(nullptr, _, _)).WillByDefault(Return(-1));

    SoundEmitterComponent emitter(&mock, nullptr);
    emitter.playSound(5, 0.5f);

    EXPECT_FALSE(emitter.isPlaying());
}

TEST(SoundEmitter, volumePropagates)
{
    NiceMock<MockSoundSystemLocal> mock;

    EXPECT_CALL(mock, startSound(nullptr, 10, 1.0f)).WillOnce(Return(1));

    SoundEmitterComponent emitter(&mock, nullptr);
    emitter.playSound(10, 1.0f);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
