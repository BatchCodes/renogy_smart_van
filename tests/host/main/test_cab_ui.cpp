// SPDX-License-Identifier: GPL-3.0-or-later
#include "cab_ui/dashboard_text.hpp"
#include "cab_ui/power_history.hpp"
#include "tests.hpp"
#include "unity.h"

using cab_ui::DataSnapshot;
using cab_ui::PowerHistory;
using charging_data::ChargeState;
using charging_data::ChargingData;
using charging_data::LinkState;

namespace {

ChargingData sample_data() {
    ChargingData data;
    data.pv_power_w = 327.0F;
    data.pv_voltage_v = 18.4F;
    data.pv_current_a = 17.8F;
    data.battery_voltage_v = 14.32F;
    data.battery_current_a = 22.9F;
    data.battery_soc_percent = 87;
    data.alternator_voltage_v = 13.71F;
    data.alternator_current_a = 2.1F;
    data.alternator_power_w = 29.0F;
    data.energy_today_wh = 1840;
    data.controller_temperature_c = 42;
    data.battery_temperature_c = -3;
    data.charge_state = ChargeState::mppt;
    return data;
}

void test_connected_values_have_units() {
    const auto text = cab_ui::format_dashboard({sample_data(), LinkState::connected, 3'000});
    TEST_ASSERT_TRUE(text.has_data);
    TEST_ASSERT_EQUAL_STRING("Updated 3 s ago", text.status.c_str());
    TEST_ASSERT_EQUAL_STRING("MPPT", text.charge_state.c_str());
    TEST_ASSERT_EQUAL_STRING("327 W", text.pv_power.c_str());
    TEST_ASSERT_EQUAL_STRING("18.4 V", text.pv_voltage.c_str());
    TEST_ASSERT_EQUAL_STRING("17.8 A", text.pv_current.c_str());
    TEST_ASSERT_EQUAL_STRING("14.32 V", text.aux_voltage.c_str());
    TEST_ASSERT_EQUAL_STRING("22.9 A", text.aux_current.c_str());
    TEST_ASSERT_EQUAL_STRING("87 %", text.aux_soc.c_str());
    TEST_ASSERT_EQUAL_STRING("13.71 V", text.starter_voltage.c_str());
    TEST_ASSERT_EQUAL_STRING("2.1 A", text.starter_current.c_str());
    TEST_ASSERT_EQUAL_STRING("29 W", text.alternator_power.c_str());
    TEST_ASSERT_EQUAL_STRING("328 W", text.charging_power.c_str());
    TEST_ASSERT_EQUAL_STRING("1.84 kWh", text.energy_today.c_str());
    TEST_ASSERT_EQUAL_STRING("42 \xC2\xB0" "C", text.controller_temperature.c_str());
    TEST_ASSERT_EQUAL_STRING("-3 \xC2\xB0" "C", text.battery_temperature.c_str());
}

void test_stale_keeps_values() {
    const auto text = cab_ui::format_dashboard({sample_data(), LinkState::stale, 45'000});
    TEST_ASSERT_TRUE(text.has_data);
    TEST_ASSERT_EQUAL_STRING("Updated 45 s ago", text.status.c_str());
}

void test_offline_hides_values() {
    const auto text = cab_ui::format_dashboard({sample_data(), LinkState::offline, 300'000});
    TEST_ASSERT_FALSE(text.has_data);
    TEST_ASSERT_TRUE(text.pv_power.empty());
    TEST_ASSERT_EQUAL_STRING("Offline, last data 5 min ago", text.status.c_str());
}

void test_waiting_for_first_data() {
    const auto text = cab_ui::format_dashboard(DataSnapshot{});
    TEST_ASSERT_FALSE(text.has_data);
    TEST_ASSERT_EQUAL_STRING("Waiting for data", text.status.c_str());
}

void test_history_keeps_one_sample_per_interval() {
    PowerHistory history(4, 1'000);
    TEST_ASSERT_TRUE(history.add(0, 10.0F));
    TEST_ASSERT_FALSE(history.add(999, 11.0F));
    TEST_ASSERT_TRUE(history.add(1'000, 12.0F));
    TEST_ASSERT_EQUAL_size_t(2, history.size());
    const auto values = history.values();
    TEST_ASSERT_EQUAL_FLOAT(10.0F, values[0]);
    TEST_ASSERT_EQUAL_FLOAT(12.0F, values[1]);
}

void test_history_drops_oldest_when_full() {
    PowerHistory history(3, 1);
    for (int index = 1; index <= 5; ++index) {
        history.add(static_cast<std::uint64_t>(index), static_cast<float>(index));
    }
    const auto values = history.values();
    TEST_ASSERT_EQUAL_size_t(3, values.size());
    TEST_ASSERT_EQUAL_FLOAT(3.0F, values[0]);
    TEST_ASSERT_EQUAL_FLOAT(5.0F, values[2]);
    TEST_ASSERT_EQUAL_FLOAT(5.0F, history.max_value());
}

void test_empty_history() {
    const PowerHistory history(10, 1'000);
    TEST_ASSERT_EQUAL_size_t(0, history.values().size());
    TEST_ASSERT_EQUAL_FLOAT(0.0F, history.max_value());
}

}  // namespace

void run_cab_ui_tests() {
    RUN_TEST(test_connected_values_have_units);
    RUN_TEST(test_stale_keeps_values);
    RUN_TEST(test_offline_hides_values);
    RUN_TEST(test_waiting_for_first_data);
    RUN_TEST(test_history_keeps_one_sample_per_interval);
    RUN_TEST(test_history_drops_oldest_when_full);
    RUN_TEST(test_empty_history);
}
