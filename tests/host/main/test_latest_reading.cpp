// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstring>

#include "charging_data/latest_reading.hpp"
#include "tests.hpp"
#include "unity.h"

using charging_data::ChargeState;
using charging_data::ChargingData;
using charging_data::FreshnessConfig;
using charging_data::LatestReading;
using charging_data::LinkState;

namespace {

constexpr FreshnessConfig kConfig{.stale_after_ms = 10'000, .offline_after_ms = 60'000};

void test_offline_before_first_reading() {
    const LatestReading reading(kConfig);
    TEST_ASSERT_TRUE(reading.link_state(0) == LinkState::offline);
    TEST_ASSERT_FALSE(reading.age_ms(5'000).has_value());
    TEST_ASSERT_FALSE(reading.data().has_value());
}

void test_link_state_follows_age() {
    LatestReading reading(kConfig);
    ChargingData data;
    data.battery_voltage_v = 13.2F;
    reading.update(data, 100'000);

    TEST_ASSERT_TRUE(reading.link_state(100'000) == LinkState::connected);
    TEST_ASSERT_TRUE(reading.link_state(109'999) == LinkState::connected);
    TEST_ASSERT_TRUE(reading.link_state(110'000) == LinkState::stale);
    TEST_ASSERT_TRUE(reading.link_state(159'999) == LinkState::stale);
    TEST_ASSERT_TRUE(reading.link_state(160'000) == LinkState::offline);
    TEST_ASSERT_EQUAL_UINT64(25'000, *reading.age_ms(125'000));
    TEST_ASSERT_EQUAL_FLOAT(13.2F, reading.data()->battery_voltage_v);
}

void test_update_resets_age() {
    LatestReading reading(kConfig);
    reading.update(ChargingData{}, 0);
    reading.update(ChargingData{}, 50'000);
    TEST_ASSERT_TRUE(reading.link_state(55'000) == LinkState::connected);
}

void test_clock_going_backwards_gives_zero_age() {
    LatestReading reading(kConfig);
    reading.update(ChargingData{}, 20'000);
    TEST_ASSERT_EQUAL_UINT64(0, *reading.age_ms(10'000));
    TEST_ASSERT_TRUE(reading.link_state(10'000) == LinkState::connected);
}

void test_to_string() {
    TEST_ASSERT_EQUAL_STRING("stale", charging_data::to_string(LinkState::stale));
    TEST_ASSERT_EQUAL_STRING("MPPT", charging_data::to_string(ChargeState::mppt));
    TEST_ASSERT_EQUAL_STRING("Unknown", charging_data::to_string(ChargeState::unknown));
}

void test_battery_charging_power() {
    ChargingData data;
    data.battery_voltage_v = 14.0F;
    data.battery_current_a = 10.0F;
    TEST_ASSERT_EQUAL_FLOAT(140.0F, data.battery_charging_power_w());
}

}  // namespace

void run_latest_reading_tests() {
    RUN_TEST(test_offline_before_first_reading);
    RUN_TEST(test_link_state_follows_age);
    RUN_TEST(test_update_resets_age);
    RUN_TEST(test_clock_going_backwards_gives_zero_age);
    RUN_TEST(test_to_string);
    RUN_TEST(test_battery_charging_power);
}
