// SPDX-License-Identifier: GPL-3.0-or-later
#include "charging_data/fake_source.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace charging_data {

namespace {

constexpr float kPeakPvPowerW = 400.0F;
constexpr float kPeakPvVoltageV = 19.0F;
constexpr float kRestingBatteryVoltageV = 12.9F;
constexpr float kAbsorptionBatteryVoltageV = 14.4F;
constexpr float kAverageLoadCurrentA = 1.5F;
constexpr std::uint32_t kPeakEnergyPerDayWh = 1800;
constexpr float kRestingStarterVoltageV = 12.6F;
constexpr float kRunningStarterVoltageV = 14.1F;
constexpr float kAlternatorChargeCurrentA = 18.0F;
// The van drives from 30 % to 45 % of the day.
constexpr float kDriveStart = 0.30F;
constexpr float kDriveEnd = 0.45F;

}  // namespace

bool FakeSource::in_outage(std::uint64_t now_ms) const {
    if (config_.outage_length_ms == 0 || config_.day_length_ms == 0) {
        return false;
    }
    const std::uint64_t time_of_day = now_ms % config_.day_length_ms;
    return time_of_day >= config_.outage_start_ms &&
           time_of_day < std::uint64_t{config_.outage_start_ms} + config_.outage_length_ms;
}

std::optional<ChargingData> FakeSource::poll(std::uint64_t now_ms) {
    if (in_outage(now_ms)) {
        return std::nullopt;
    }
    return reading_at(now_ms);
}

ChargingData FakeSource::reading_at(std::uint64_t now_ms) const {
    const std::uint32_t day_length = std::max<std::uint32_t>(config_.day_length_ms, 1);
    const float day_fraction =
        static_cast<float>(now_ms % day_length) / static_cast<float>(day_length);

    // The sun is up for the first half of the day.
    const float sun = std::max(0.0F, std::sin(2.0F * std::numbers::pi_v<float> * day_fraction));
    const bool is_day = sun > 0.0F;

    // A small ripple, so the battery values change at every refresh, also at night.
    const float seconds = static_cast<float>(now_ms % 3'600'000) / 1000.0F;
    const float ripple = 0.5F * std::sin(seconds * 0.9F) + 0.5F * std::sin(seconds * 2.3F);

    ChargingData data;
    data.pv_power_w = std::round(kPeakPvPowerW * sun);
    data.pv_voltage_v = is_day ? kPeakPvVoltageV - 1.5F * (1.0F - sun) : 0.0F;
    data.pv_current_a = data.pv_voltage_v > 0.0F ? data.pv_power_w / data.pv_voltage_v : 0.0F;

    data.battery_voltage_v = kRestingBatteryVoltageV +
                             (kAbsorptionBatteryVoltageV - kRestingBatteryVoltageV) * sun +
                             0.03F * ripple;
    const bool is_driving = day_fraction >= kDriveStart && day_fraction < kDriveEnd;
    data.alternator_voltage_v =
        (is_driving ? kRunningStarterVoltageV : kRestingStarterVoltageV) + 0.02F * ripple;
    data.alternator_current_a = is_driving ? kAlternatorChargeCurrentA + 0.4F * ripple : 0.0F;
    data.alternator_power_w = std::round(data.alternator_voltage_v * data.alternator_current_a);

    data.battery_current_a = (data.pv_power_w + data.alternator_power_w) / data.battery_voltage_v -
                             kAverageLoadCurrentA + 0.3F * ripple;

    data.controller_temperature_c = static_cast<std::int8_t>(18 + std::lround(22.0F * sun));
    data.battery_temperature_c = static_cast<std::int8_t>(16 + std::lround(6.0F * sun));

    // Energy climbs while the sun is up, then stays at the day's total.
    const float sun_progress = std::min(day_fraction * 2.0F, 1.0F);
    const float energy_curve =
        (1.0F - std::cos(std::numbers::pi_v<float> * sun_progress)) / 2.0F;
    data.energy_today_wh = static_cast<std::uint32_t>(std::lround(kPeakEnergyPerDayWh * energy_curve));

    if (is_driving) {
        data.charge_state = ChargeState::alternator_direct;
    } else if (!is_day) {
        data.charge_state = ChargeState::deactivated;
    } else if (sun > 0.9F) {
        data.charge_state = ChargeState::boost;
    } else if (day_fraction > 0.25F) {
        data.charge_state = ChargeState::floating;
    } else {
        data.charge_state = ChargeState::mppt;
    }
    return data;
}

}  // namespace charging_data
