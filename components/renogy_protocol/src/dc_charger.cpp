// SPDX-License-Identifier: GPL-3.0-or-later
#include "renogy_protocol/dc_charger.hpp"

namespace renogy_protocol {

namespace {

using charging_data::ChargeState;

// Register offsets from 0x0100.
constexpr std::size_t kSoc = 0x00;
constexpr std::size_t kBatteryVoltage = 0x01;       // 0.1 V
constexpr std::size_t kChargeCurrent = 0x02;        // 0.01 A, solar + alternator
constexpr std::size_t kTemperatures = 0x03;         // High byte controller, low byte battery
constexpr std::size_t kAlternatorVoltage = 0x04;    // 0.1 V
constexpr std::size_t kAlternatorCurrent = 0x05;    // 0.01 A
constexpr std::size_t kAlternatorPower = 0x06;      // 1 W
constexpr std::size_t kPvVoltage = 0x07;            // 0.1 V
constexpr std::size_t kPvCurrent = 0x08;            // 0.01 A
constexpr std::size_t kPvPower = 0x09;              // 1 W
constexpr std::size_t kEnergyToday = 0x13;          // 1 Wh

ChargeState to_charge_state(std::uint8_t raw) {
    switch (raw) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
            return static_cast<ChargeState>(raw);
        default:
            return ChargeState::unknown;
    }
}

}  // namespace

std::int8_t decode_temperature(std::uint8_t raw) {
    const auto magnitude = static_cast<std::int8_t>(raw & 0x7F);
    return (raw & 0x80) != 0 ? static_cast<std::int8_t>(-magnitude) : magnitude;
}

FrameError decode_charging_block(std::span<const std::uint8_t> frame, charging_data::ChargingData& data) {
    const FrameError error = check_read_response(frame, kChargingBlock.register_count);
    if (error != FrameError::none) {
        return error;
    }
    const auto value = [&frame](std::size_t index) { return register_value(frame, index); };

    data.battery_soc_percent = static_cast<std::uint8_t>(value(kSoc) > 100 ? 100 : value(kSoc));
    data.battery_voltage_v = value(kBatteryVoltage) * 0.1F;
    // The charger reports the charge current only, so it is never negative.
    data.battery_current_a = value(kChargeCurrent) * 0.01F;
    data.controller_temperature_c = decode_temperature(static_cast<std::uint8_t>(value(kTemperatures) >> 8));
    data.battery_temperature_c = decode_temperature(static_cast<std::uint8_t>(value(kTemperatures) & 0xFF));
    data.alternator_voltage_v = value(kAlternatorVoltage) * 0.1F;
    data.alternator_current_a = value(kAlternatorCurrent) * 0.01F;
    data.alternator_power_w = static_cast<float>(value(kAlternatorPower));
    data.pv_voltage_v = value(kPvVoltage) * 0.1F;
    data.pv_current_a = value(kPvCurrent) * 0.01F;
    data.pv_power_w = static_cast<float>(value(kPvPower));
    data.energy_today_wh = value(kEnergyToday);
    return FrameError::none;
}

FrameError decode_state_block(std::span<const std::uint8_t> frame, charging_data::ChargingData& data) {
    const FrameError error = check_read_response(frame, kStateBlock.register_count);
    if (error != FrameError::none) {
        return error;
    }
    // The low byte of 0x0120 is the charging state. The Python reference reads the frame's
    // byte count field here, which looks like a bug, so this follows the register sheet.
    data.charge_state = to_charge_state(static_cast<std::uint8_t>(register_value(frame, 0) & 0xFF));
    return FrameError::none;
}

}  // namespace renogy_protocol
