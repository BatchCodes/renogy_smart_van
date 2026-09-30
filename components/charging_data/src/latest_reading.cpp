// SPDX-License-Identifier: GPL-3.0-or-later
#include "charging_data/latest_reading.hpp"

namespace charging_data {

void LatestReading::update(const ChargingData& data, std::uint64_t now_ms) {
    data_ = data;
    updated_at_ms_ = now_ms;
}

std::optional<std::uint64_t> LatestReading::age_ms(std::uint64_t now_ms) const {
    if (!data_) {
        return std::nullopt;
    }
    // A clock that goes backwards must not give a huge unsigned age.
    return now_ms >= updated_at_ms_ ? now_ms - updated_at_ms_ : 0;
}

LinkState LatestReading::link_state(std::uint64_t now_ms) const {
    const auto age = age_ms(now_ms);
    if (!age || *age >= config_.offline_after_ms) {
        return LinkState::offline;
    }
    if (*age >= config_.stale_after_ms) {
        return LinkState::stale;
    }
    return LinkState::connected;
}

const char* to_string(LinkState state) {
    switch (state) {
        case LinkState::connected:
            return "connected";
        case LinkState::stale:
            return "stale";
        case LinkState::offline:
            return "offline";
    }
    return "unknown";
}

const char* to_string(ChargeState state) {
    switch (state) {
        case ChargeState::deactivated:
            return "Off";
        case ChargeState::activated:
            return "On";
        case ChargeState::mppt:
            return "MPPT";
        case ChargeState::equalizing:
            return "Equalize";
        case ChargeState::boost:
            return "Boost";
        case ChargeState::floating:
            return "Float";
        case ChargeState::current_limiting:
            return "Current limit";
        case ChargeState::alternator_direct:
            return "Alternator";
        case ChargeState::unknown:
            break;
    }
    return "Unknown";
}

}  // namespace charging_data
