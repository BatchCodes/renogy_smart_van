// SPDX-License-Identifier: GPL-3.0-or-later
#include "charging_data/fake_source.hpp"
#include "tests.hpp"
#include "unity.h"

using charging_data::ChargeState;
using charging_data::FakeSource;
using charging_data::FakeSourceConfig;

namespace {

constexpr FakeSourceConfig kConfig{
    .day_length_ms = 100'000,
    .outage_start_ms = 80'000,
    .outage_length_ms = 10'000,
};

void test_noon_has_peak_power() {
    const FakeSource source(kConfig);
    const auto noon = source.reading_at(25'000);
    TEST_ASSERT_EQUAL_FLOAT(400.0F, noon.pv_power_w);
    TEST_ASSERT_TRUE(noon.charge_state == ChargeState::boost);
    TEST_ASSERT_FLOAT_WITHIN(0.05F, 14.4F, noon.battery_voltage_v);
    TEST_ASSERT_TRUE(noon.battery_current_a > 0.0F);
}

void test_night_has_no_pv() {
    const FakeSource source(kConfig);
    const auto night = source.reading_at(75'000);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, night.pv_power_w);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, night.pv_voltage_v);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, night.pv_current_a);
    TEST_ASSERT_TRUE(night.charge_state == ChargeState::deactivated);
    TEST_ASSERT_TRUE(night.battery_current_a < 0.0F);
}

void test_energy_rises_then_holds() {
    const FakeSource source(kConfig);
    const auto morning = source.reading_at(10'000).energy_today_wh;
    const auto noon = source.reading_at(25'000).energy_today_wh;
    const auto evening = source.reading_at(50'000).energy_today_wh;
    const auto night = source.reading_at(90'000).energy_today_wh;
    TEST_ASSERT_TRUE(morning < noon);
    TEST_ASSERT_TRUE(noon < evening);
    TEST_ASSERT_EQUAL_UINT32(evening, night);
    TEST_ASSERT_EQUAL_UINT32(0, source.reading_at(0).energy_today_wh);
}

void test_drive_charges_from_alternator() {
    const FakeSource source(kConfig);
    const auto driving = source.reading_at(35'000);
    TEST_ASSERT_TRUE(driving.charge_state == ChargeState::alternator_direct);
    TEST_ASSERT_TRUE(driving.alternator_current_a > 0.0F);
    TEST_ASSERT_TRUE(driving.alternator_voltage_v > 13.5F);
    const auto parked = source.reading_at(10'000);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, parked.alternator_current_a);
    TEST_ASSERT_TRUE(parked.alternator_voltage_v > 12.0F);
}

void test_night_values_change_between_refreshes() {
    const FakeSource source(kConfig);
    const auto first = source.reading_at(70'000);
    const auto second = source.reading_at(75'000);
    TEST_ASSERT_TRUE(first.battery_voltage_v != second.battery_voltage_v);
    TEST_ASSERT_TRUE(first.battery_current_a != second.battery_current_a);
}

void test_outage_returns_nothing() {
    FakeSource source(kConfig);
    TEST_ASSERT_TRUE(source.poll(79'999).has_value());
    TEST_ASSERT_FALSE(source.poll(80'000).has_value());
    TEST_ASSERT_FALSE(source.poll(189'999).has_value());
    TEST_ASSERT_TRUE(source.poll(190'000).has_value());
}

void test_outage_can_be_disabled() {
    FakeSource source(FakeSourceConfig{.day_length_ms = 100'000, .outage_length_ms = 0});
    for (std::uint64_t time_ms = 0; time_ms < 200'000; time_ms += 1'000) {
        TEST_ASSERT_TRUE(source.poll(time_ms).has_value());
    }
}

void test_values_depend_only_on_time() {
    FakeSource first(kConfig);
    FakeSource second(kConfig);
    const auto first_reading = first.poll(12'345);
    const auto second_reading = second.poll(112'345);
    TEST_ASSERT_EQUAL_FLOAT(first_reading->pv_power_w, second_reading->pv_power_w);
    TEST_ASSERT_EQUAL_UINT32(first_reading->energy_today_wh, second_reading->energy_today_wh);
}

void test_zero_day_length_does_not_crash() {
    FakeSource source(FakeSourceConfig{.day_length_ms = 0});
    TEST_ASSERT_TRUE(source.poll(1'000).has_value());
}

}  // namespace

void run_fake_source_tests() {
    RUN_TEST(test_noon_has_peak_power);
    RUN_TEST(test_night_has_no_pv);
    RUN_TEST(test_energy_rises_then_holds);
    RUN_TEST(test_drive_charges_from_alternator);
    RUN_TEST(test_night_values_change_between_refreshes);
    RUN_TEST(test_outage_returns_nothing);
    RUN_TEST(test_outage_can_be_disabled);
    RUN_TEST(test_values_depend_only_on_time);
    RUN_TEST(test_zero_day_length_does_not_crash);
}
