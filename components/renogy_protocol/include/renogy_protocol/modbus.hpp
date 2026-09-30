// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace renogy_protocol {

// The BT-2 forwards Modbus RTU frames between BLE and the charger's RS485 port.
inline constexpr std::uint8_t kReadHoldingRegisters = 0x03;
inline constexpr std::uint8_t kExceptionFlag = 0x80;
// Device ID 255 reaches the charger that is directly connected to the BT-2.
inline constexpr std::uint8_t kDefaultDeviceId = 0xFF;

using ReadRequest = std::array<std::uint8_t, 8>;

// CRC-16/Modbus. The frame carries it low byte first.
[[nodiscard]] std::uint16_t crc16_modbus(std::span<const std::uint8_t> bytes);

[[nodiscard]] ReadRequest build_read_request(std::uint8_t device_id, std::uint16_t first_register,
                                             std::uint16_t register_count);

enum class FrameError : std::uint8_t {
    none,
    too_short,
    bad_crc,
    exception,       // The charger answered with a Modbus exception.
    wrong_function,
    wrong_length,    // The frame does not hold the expected number of registers.
};

[[nodiscard]] const char* to_string(FrameError error);

// Checks a complete response to a read request for register_count registers.
[[nodiscard]] FrameError check_read_response(std::span<const std::uint8_t> frame,
                                             std::uint16_t register_count);

// The value of register index (0 = first register read) in a checked read response.
[[nodiscard]] std::uint16_t register_value(std::span<const std::uint8_t> frame, std::size_t index);

// Joins BLE notifications into one Modbus response. A response longer than one
// notification (20 bytes with the default MTU) arrives in several parts.
class ResponseAssembler {
public:
    enum class Status : std::uint8_t { incomplete, complete, overflow };

    void reset() { length_ = 0; }
    Status add(std::span<const std::uint8_t> chunk);
    [[nodiscard]] std::span<const std::uint8_t> frame() const { return {buffer_.data(), length_}; }

private:
    [[nodiscard]] std::size_t expected_length() const;

    std::array<std::uint8_t, 260> buffer_{};
    std::size_t length_ = 0;
};

}  // namespace renogy_protocol
