// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>

namespace charging_data {

// Charging state reported by a Renogy DC-DC charger with MPPT (register 288).
// The numeric values match the charger's register values.
enum class ChargeState : std::uint8_t {
    deactivated = 0,
    activated = 1,
    mppt = 2,
    equalizing = 3,
    boost = 4,
    floating = 5,
    current_limiting = 6,
    alternator_direct = 8,
    unknown = 0xFF,
};

// One set of values read from the charger. "Alternator" is the starter battery input.
struct ChargingData {
    float battery_voltage_v = 0.0F;
    float battery_current_a = 0.0F;
    float pv_voltage_v = 0.0F;
    float pv_current_a = 0.0F;
    float pv_power_w = 0.0F;
    float alternator_voltage_v = 0.0F;
    float alternator_current_a = 0.0F;
    float alternator_power_w = 0.0F;
    std::int8_t controller_temperature_c = 0;
    std::int8_t battery_temperature_c = 0;
    std::uint32_t energy_today_wh = 0;
    std::uint8_t battery_soc_percent = 0;  // State of charge that the charger estimates.
    ChargeState charge_state = ChargeState::unknown;

    [[nodiscard]] float battery_charging_power_w() const {
        return battery_voltage_v * battery_current_a;
    }
};

// Where the values come from. The display code depends only on this interface,
// so a fake source, the BT-2 source and a relay source are interchangeable.
class ChargingDataSource {
public:
    virtual ~ChargingDataSource() = default;

    // The owner calls this periodically. It returns a new reading when one is
    // available, and std::nullopt when there is nothing new.
    virtual std::optional<ChargingData> poll(std::uint64_t now_ms) = 0;
};

[[nodiscard]] const char* to_string(ChargeState state);

}  // namespace charging_data
