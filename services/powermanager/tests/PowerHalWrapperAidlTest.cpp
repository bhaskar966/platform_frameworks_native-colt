/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "PowerHalWrapperAidlTest"

#include <android/hardware/power/Boost.h>
#include <android/hardware/power/Mode.h>
#include <binder/IServiceManager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <powermanager/PowerHalWrapper.h>
#include <utils/Log.h>

#include <thread>

using android::binder::Status;
using android::hardware::power::Boost;
using android::hardware::power::IPower;
using android::hardware::power::Mode;

using namespace android;
using namespace android::power;
using namespace std::chrono_literals;
using namespace testing;

// -------------------------------------------------------------------------------------------------

class MockIPower : public IPower {
public:
    MOCK_METHOD(Status, isBoostSupported, (Boost boost, bool* ret), (override));
    MOCK_METHOD(Status, setBoost, (Boost boost, int32_t durationMs), (override));
    MOCK_METHOD(Status, isModeSupported, (Mode mode, bool* ret), (override));
    MOCK_METHOD(Status, setMode, (Mode mode, bool enabled), (override));
    MOCK_METHOD(int32_t, getInterfaceVersion, (), (override));
    MOCK_METHOD(std::string, getInterfaceHash, (), (override));
    MOCK_METHOD(IBinder*, onAsBinder, (), (override));
};

// -------------------------------------------------------------------------------------------------

class PowerHalWrapperAidlTest : public Test {
public:
    void SetUp() override;

protected:
    std::unique_ptr<HalWrapper> mWrapper = nullptr;
    sp<StrictMock<MockIPower>> mMockHal = nullptr;
};

// -------------------------------------------------------------------------------------------------

void PowerHalWrapperAidlTest::SetUp() {
    mMockHal = new StrictMock<MockIPower>();
    mWrapper = std::make_unique<AidlHalWrapper>(mMockHal);
    ASSERT_NE(mWrapper, nullptr);
}

// -------------------------------------------------------------------------------------------------

TEST_F(PowerHalWrapperAidlTest, TestSetBoostSuccessful) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), isBoostSupported(Eq(Boost::DISPLAY_UPDATE_IMMINENT), _))
                .Times(Exactly(1))
                .WillRepeatedly(DoAll(SetArgPointee<1>(true), Return(Status())));
        EXPECT_CALL(*mMockHal.get(), setBoost(Eq(Boost::DISPLAY_UPDATE_IMMINENT), Eq(100)))
                .Times(Exactly(1));
    }

    auto result = mWrapper->setBoost(Boost::DISPLAY_UPDATE_IMMINENT, 100);
    ASSERT_EQ(HalResult::SUCCESSFUL, result);
}

TEST_F(PowerHalWrapperAidlTest, TestSetBoostFailed) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), isBoostSupported(Eq(Boost::INTERACTION), _))
                .Times(Exactly(1))
                .WillRepeatedly(DoAll(SetArgPointee<1>(true), Return(Status())));
        EXPECT_CALL(*mMockHal.get(), setBoost(Eq(Boost::INTERACTION), Eq(100)))
                .Times(Exactly(1))
                .WillRepeatedly(Return(Status::fromExceptionCode(-1)));
        EXPECT_CALL(*mMockHal.get(), isBoostSupported(Eq(Boost::DISPLAY_UPDATE_IMMINENT), _))
                .Times(Exactly(1))
                .WillRepeatedly(Return(Status::fromExceptionCode(-1)));
    }

    auto result = mWrapper->setBoost(Boost::INTERACTION, 100);
    ASSERT_EQ(HalResult::FAILED, result);
    result = mWrapper->setBoost(Boost::DISPLAY_UPDATE_IMMINENT, 1000);
    ASSERT_EQ(HalResult::FAILED, result);
}

TEST_F(PowerHalWrapperAidlTest, TestSetBoostUnsupported) {
    EXPECT_CALL(*mMockHal.get(), isBoostSupported(Eq(Boost::INTERACTION), _))
            .Times(Exactly(1))
            .WillRepeatedly(DoAll(SetArgPointee<1>(false), Return(Status())));

    auto result = mWrapper->setBoost(Boost::INTERACTION, 1000);
    ASSERT_EQ(HalResult::UNSUPPORTED, result);
    result = mWrapper->setBoost(Boost::CAMERA_SHOT, 10);
    ASSERT_EQ(HalResult::UNSUPPORTED, result);
}

TEST_F(PowerHalWrapperAidlTest, TestSetBoostMultiThreadCheckSupportedOnlyOnce) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), isBoostSupported(Eq(Boost::INTERACTION), _))
                .Times(Exactly(1))
                .WillRepeatedly(DoAll(SetArgPointee<1>(true), Return(Status())));
        EXPECT_CALL(*mMockHal.get(), setBoost(Eq(Boost::INTERACTION), Eq(100))).Times(Exactly(10));
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.push_back(std::thread([&]() {
            auto result = mWrapper->setBoost(Boost::INTERACTION, 100);
            ASSERT_EQ(HalResult::SUCCESSFUL, result);
        }));
    }
    std::for_each(threads.begin(), threads.end(), [](std::thread& t) { t.join(); });
}

TEST_F(PowerHalWrapperAidlTest, TestSetModeSuccessful) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), isModeSupported(Eq(Mode::DISPLAY_INACTIVE), _))
                .Times(Exactly(1))
                .WillRepeatedly(DoAll(SetArgPointee<1>(true), Return(Status())));
        EXPECT_CALL(*mMockHal.get(), setMode(Eq(Mode::DISPLAY_INACTIVE), Eq(false)))
                .Times(Exactly(1));
    }

    auto result = mWrapper->setMode(Mode::DISPLAY_INACTIVE, false);
    ASSERT_EQ(HalResult::SUCCESSFUL, result);
}

TEST_F(PowerHalWrapperAidlTest, TestSetModeFailed) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), isModeSupported(Eq(Mode::LAUNCH), _))
                .Times(Exactly(1))
                .WillRepeatedly(DoAll(SetArgPointee<1>(true), Return(Status())));
        EXPECT_CALL(*mMockHal.get(), setMode(Eq(Mode::LAUNCH), Eq(true)))
                .Times(Exactly(1))
                .WillRepeatedly(Return(Status::fromExceptionCode(-1)));
        EXPECT_CALL(*mMockHal.get(), isModeSupported(Eq(Mode::DISPLAY_INACTIVE), _))
                .Times(Exactly(1))
                .WillRepeatedly(Return(Status::fromExceptionCode(-1)));
    }

    auto result = mWrapper->setMode(Mode::LAUNCH, true);
    ASSERT_EQ(HalResult::FAILED, result);
    result = mWrapper->setMode(Mode::DISPLAY_INACTIVE, false);
    ASSERT_EQ(HalResult::FAILED, result);
}

TEST_F(PowerHalWrapperAidlTest, TestSetModeUnsupported) {
    EXPECT_CALL(*mMockHal.get(), isModeSupported(Eq(Mode::LAUNCH), _))
            .Times(Exactly(1))
            .WillRepeatedly(DoAll(SetArgPointee<1>(false), Return(Status())));

    auto result = mWrapper->setMode(Mode::LAUNCH, true);
    ASSERT_EQ(HalResult::UNSUPPORTED, result);
    result = mWrapper->setMode(Mode::CAMERA_STREAMING_HIGH, true);
    ASSERT_EQ(HalResult::UNSUPPORTED, result);
}

TEST_F(PowerHalWrapperAidlTest, TestSetModeMultiThreadCheckSupportedOnlyOnce) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), isModeSupported(Eq(Mode::LAUNCH), _))
                .Times(Exactly(1))
                .WillRepeatedly(DoAll(SetArgPointee<1>(true), Return(Status())));
        EXPECT_CALL(*mMockHal.get(), setMode(Eq(Mode::LAUNCH), Eq(false))).Times(Exactly(10));
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.push_back(std::thread([&]() {
            auto result = mWrapper->setMode(Mode::LAUNCH, false);
            ASSERT_EQ(HalResult::SUCCESSFUL, result);
        }));
    }
    std::for_each(threads.begin(), threads.end(), [](std::thread& t) { t.join(); });
}