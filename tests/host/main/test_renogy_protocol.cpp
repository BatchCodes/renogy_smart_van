// SPDX-License-Identifier: GPL-3.0-or-later
//
// The expected frames and CRCs were made with crc16_modbus() and the parsers in
// cyrils/renogy-bt (Utils.py, DCChargerClient.py), so these tests check that the port
// matches the reference.

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "renogy_protocol/dc_charger.hpp"
#include "renogy_protocol/modbus.hpp"
#include "tests.hpp"
#include "unity.h"

using charging_data::ChargeState;
using charging_data::ChargingData;
using renogy_protocol::FrameError;
using renogy_protocol::ResponseAssembler;

namespace {

// Response to a read of 30 registers from 0x0100: SOC 87 %, 13.2 V, 17.02 A,
// controller 18 °C, battery -5 °C, alternator 12.9 V 2.10 A 27 W, PV 18.6 V 8.12 A 151 W,
// 336 Wh today.
const std::vector<std::uint8_t> kChargingFrame{
    0xFF, 0x03, 0x3C, 0x00, 0x57, 0x00, 0x84, 0x06, 0xA6, 0x12, 0x85, 0x00, 0x81, 0x00, 0xD2,
    0x00, 0x1B, 0x00, 0xBA, 0x03, 0x2C, 0x00, 0x97, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x50, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xE6, 0x6E,
};

// Response to a read of 3 registers from 0x0120: charging state 2 (MPPT).
const std::vector<std::uint8_t> kStateFrame{0xFF, 0x03, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x10, 0xD1};

// Modbus exception 2 (illegal data address).
const std::vector<std::uint8_t> kExceptionFrame{0xFF, 0x83, 0x02, 0xA1, 0x01};

void test_read_requests_match_reference() {
    const std::array<std::uint8_t, 8> model{0xFF, 0x03, 0x00, 0x0C, 0x00, 0x02, 0x11, 0xD6};
    const std::array<std::uint8_t, 8> charging{0xFF, 0x03, 0x01, 0x00, 0x00, 0x1E, 0xD1, 0xE0};
    const std::array<std::uint8_t, 8> state{0xFF, 0x03, 0x01, 0x20, 0x00, 0x03, 0x10, 0x23};
    TEST_ASSERT_EQUAL_HEX8_ARRAY(model.data(), renogy_protocol::build_read_request(0xFF, 0x000C, 2).data(), 8);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(charging.data(), renogy_protocol::build_read_request(0xFF, 0x0100, 30).data(), 8);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(state.data(), renogy_protocol::build_read_request(0xFF, 0x0120, 3).data(), 8);
}

void test_crc_standard_check_value() {
    const std::array<std::uint8_t, 9> check{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    TEST_ASSERT_EQUAL_HEX16(0x4B37, renogy_protocol::crc16_modbus(check));
}

void test_decode_charging_block() {
    ChargingData data;
    TEST_ASSERT_TRUE(renogy_protocol::decode_charging_block(kChargingFrame, data) == FrameError::none);
    TEST_ASSERT_EQUAL_UINT8(87, data.battery_soc_percent);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 13.2F, data.battery_voltage_v);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 17.02F, data.battery_current_a);
    TEST_ASSERT_EQUAL_INT8(18, data.controller_temperature_c);
    TEST_ASSERT_EQUAL_INT8(-5, data.battery_temperature_c);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 12.9F, data.alternator_voltage_v);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 2.1F, data.alternator_current_a);
    TEST_ASSERT_EQUAL_FLOAT(27.0F, data.alternator_power_w);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 18.6F, data.pv_voltage_v);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 8.12F, data.pv_current_a);
    TEST_ASSERT_EQUAL_FLOAT(151.0F, data.pv_power_w);
    TEST_ASSERT_EQUAL_UINT32(336, data.energy_today_wh);
}

void test_decode_state_block() {
    ChargingData data;
    TEST_ASSERT_TRUE(renogy_protocol::decode_state_block(kStateFrame, data) == FrameError::none);
    TEST_ASSERT_TRUE(data.charge_state == ChargeState::mppt);
}

void test_corrupt_frame_is_rejected_and_data_kept() {
    auto frame = kChargingFrame;
    frame[6] ^= 0x01;
    ChargingData data;
    data.battery_voltage_v = 12.5F;
    TEST_ASSERT_TRUE(renogy_protocol::decode_charging_block(frame, data) == FrameError::bad_crc);
    TEST_ASSERT_EQUAL_FLOAT(12.5F, data.battery_voltage_v);
}

void test_exception_and_wrong_block() {
    ChargingData data;
    TEST_ASSERT_TRUE(renogy_protocol::decode_charging_block(kExceptionFrame, data) == FrameError::exception);
    TEST_ASSERT_TRUE(renogy_protocol::decode_charging_block(kStateFrame, data) == FrameError::wrong_length);
    const std::array<std::uint8_t, 3> short_frame{0xFF, 0x03, 0x00};
    TEST_ASSERT_TRUE(renogy_protocol::check_read_response(short_frame, 0) == FrameError::too_short);
}

void test_temperature_sign_and_magnitude() {
    TEST_ASSERT_EQUAL_INT8(25, renogy_protocol::decode_temperature(0x19));
    TEST_ASSERT_EQUAL_INT8(-25, renogy_protocol::decode_temperature(0x99));
    TEST_ASSERT_EQUAL_INT8(0, renogy_protocol::decode_temperature(0x00));
}

void test_unknown_charge_state() {
    auto frame = std::vector<std::uint8_t>{0xFF, 0x03, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00};
    const std::uint16_t crc = renogy_protocol::crc16_modbus(frame);
    frame.push_back(static_cast<std::uint8_t>(crc & 0xFF));
    frame.push_back(static_cast<std::uint8_t>(crc >> 8));
    ChargingData data;
    TEST_ASSERT_TRUE(renogy_protocol::decode_state_block(frame, data) == FrameError::none);
    TEST_ASSERT_TRUE(data.charge_state == ChargeState::unknown);
}

void test_assembler_joins_20_byte_notifications() {
    ResponseAssembler assembler;
    const std::span<const std::uint8_t> frame(kChargingFrame);
    for (std::size_t offset = 0; offset < frame.size(); offset += 20) {
        const std::size_t length = std::min<std::size_t>(20, frame.size() - offset);
        const auto status = assembler.add(frame.subspan(offset, length));
        const bool last = offset + length == frame.size();
        TEST_ASSERT_TRUE(status == (last ? ResponseAssembler::Status::complete
                                         : ResponseAssembler::Status::incomplete));
    }
    TEST_ASSERT_EQUAL_size_t(kChargingFrame.size(), assembler.frame().size());
    TEST_ASSERT_EQUAL_HEX8_ARRAY(kChargingFrame.data(), assembler.frame().data(), kChargingFrame.size());
}

void test_assembler_exception_frame_and_overflow() {
    ResponseAssembler assembler;
    TEST_ASSERT_TRUE(assembler.add(kExceptionFrame) == ResponseAssembler::Status::complete);
    assembler.reset();
    auto too_long = kStateFrame;
    too_long.push_back(0x00);
    TEST_ASSERT_TRUE(assembler.add(too_long) == ResponseAssembler::Status::overflow);
    TEST_ASSERT_EQUAL_size_t(0, assembler.frame().size());
}

}  // namespace

void run_renogy_protocol_tests() {
    RUN_TEST(test_read_requests_match_reference);
    RUN_TEST(test_crc_standard_check_value);
    RUN_TEST(test_decode_charging_block);
    RUN_TEST(test_decode_state_block);
    RUN_TEST(test_corrupt_frame_is_rejected_and_data_kept);
    RUN_TEST(test_exception_and_wrong_block);
    RUN_TEST(test_temperature_sign_and_magnitude);
    RUN_TEST(test_unknown_charge_state);
    RUN_TEST(test_assembler_joins_20_byte_notifications);
    RUN_TEST(test_assembler_exception_frame_and_overflow);
}
