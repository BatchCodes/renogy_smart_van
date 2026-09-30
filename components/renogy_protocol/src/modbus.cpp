// SPDX-License-Identifier: GPL-3.0-or-later
#include "renogy_protocol/modbus.hpp"

#include <algorithm>

namespace renogy_protocol {

namespace {

constexpr std::size_t kHeaderBytes = 3;  // Device ID, function, byte count.
constexpr std::size_t kCrcBytes = 2;
constexpr std::size_t kExceptionFrameBytes = 5;

}  // namespace

std::uint16_t crc16_modbus(std::span<const std::uint8_t> bytes) {
    std::uint16_t crc = 0xFFFF;
    for (const std::uint8_t byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) != 0 ? static_cast<std::uint16_t>((crc >> 1) ^ 0xA001U)
                                  : static_cast<std::uint16_t>(crc >> 1);
        }
    }
    return crc;
}

ReadRequest build_read_request(std::uint8_t device_id, std::uint16_t first_register,
                               std::uint16_t register_count) {
    ReadRequest request{
        device_id,
        kReadHoldingRegisters,
        static_cast<std::uint8_t>(first_register >> 8),
        static_cast<std::uint8_t>(first_register & 0xFF),
        static_cast<std::uint8_t>(register_count >> 8),
        static_cast<std::uint8_t>(register_count & 0xFF),
        0,
        0,
    };
    const std::uint16_t crc = crc16_modbus(std::span(request).first(6));
    request[6] = static_cast<std::uint8_t>(crc & 0xFF);
    request[7] = static_cast<std::uint8_t>(crc >> 8);
    return request;
}

FrameError check_read_response(std::span<const std::uint8_t> frame, std::uint16_t register_count) {
    if (frame.size() < kExceptionFrameBytes) {
        return FrameError::too_short;
    }
    const std::uint16_t expected_crc = crc16_modbus(frame.first(frame.size() - kCrcBytes));
    const std::uint16_t frame_crc =
        static_cast<std::uint16_t>(frame[frame.size() - 2] | (frame[frame.size() - 1] << 8));
    if (expected_crc != frame_crc) {
        return FrameError::bad_crc;
    }
    if (frame[1] == (kReadHoldingRegisters | kExceptionFlag)) {
        return FrameError::exception;
    }
    if (frame[1] != kReadHoldingRegisters) {
        return FrameError::wrong_function;
    }
    const std::size_t data_bytes = static_cast<std::size_t>(register_count) * 2;
    if (frame[2] != data_bytes || frame.size() != kHeaderBytes + data_bytes + kCrcBytes) {
        return FrameError::wrong_length;
    }
    return FrameError::none;
}

std::uint16_t register_value(std::span<const std::uint8_t> frame, std::size_t index) {
    const std::size_t offset = kHeaderBytes + index * 2;
    return static_cast<std::uint16_t>((frame[offset] << 8) | frame[offset + 1]);
}

std::size_t ResponseAssembler::expected_length() const {
    if (length_ < kHeaderBytes) {
        return 0;
    }
    if ((buffer_[1] & kExceptionFlag) != 0) {
        return kExceptionFrameBytes;
    }
    return kHeaderBytes + buffer_[2] + kCrcBytes;
}

ResponseAssembler::Status ResponseAssembler::add(std::span<const std::uint8_t> chunk) {
    if (length_ + chunk.size() > buffer_.size()) {
        length_ = 0;
        return Status::overflow;
    }
    std::copy(chunk.begin(), chunk.end(), buffer_.begin() + static_cast<std::ptrdiff_t>(length_));
    length_ += chunk.size();

    const std::size_t expected = expected_length();
    if (expected == 0 || length_ < expected) {
        return Status::incomplete;
    }
    if (length_ > expected) {
        length_ = 0;
        return Status::overflow;
    }
    return Status::complete;
}

const char* to_string(FrameError error) {
    switch (error) {
        case FrameError::none:
            return "none";
        case FrameError::too_short:
            return "too short";
        case FrameError::bad_crc:
            return "bad CRC";
        case FrameError::exception:
            return "Modbus exception";
        case FrameError::wrong_function:
            return "wrong function";
        case FrameError::wrong_length:
            return "wrong length";
    }
    return "unknown";
}

}  // namespace renogy_protocol
