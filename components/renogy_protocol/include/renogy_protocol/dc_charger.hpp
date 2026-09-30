// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <span>

#include "charging_data/charging_data.hpp"
#include "renogy_protocol/modbus.hpp"

namespace renogy_protocol {

// Register blocks of a Renogy DC-DC charger with MPPT (RBC50D1S, DCC50S). The map comes
// from cyrils/renogy-bt (DCChargerClient.py), neilsheps/Renogy-BT2-Reader and the
// "Renogy DCC series register mappings" sheet in that repository.
struct RegisterBlock {
    std::uint16_t first_register;
    std::uint16_t register_count;
};

// Model name. The Renogy app reads this first after it connects.
inline constexpr RegisterBlock kModelBlock{0x000C, 8};
// Battery, alternator, solar and daily values, 0x0100 to 0x011D.
inline constexpr RegisterBlock kChargingBlock{0x0100, 30};
// Charging state and error flags, 0x0120 to 0x0122.
inline constexpr RegisterBlock kStateBlock{0x0120, 3};

// Decodes a checked response to kChargingBlock into data. Other fields are unchanged.
FrameError decode_charging_block(std::span<const std::uint8_t> frame, charging_data::ChargingData& data);

// Decodes a checked response to kStateBlock into data.charge_state.
FrameError decode_state_block(std::span<const std::uint8_t> frame, charging_data::ChargingData& data);

// Temperatures are sign and magnitude: bit 7 is the sign, bits 0 to 6 the value in °C.
[[nodiscard]] std::int8_t decode_temperature(std::uint8_t raw);

}  // namespace renogy_protocol
