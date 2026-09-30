// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>

#include "charging_data/charging_data.hpp"

namespace charging_data {

// How current the latest reading is.
enum class LinkState : std::uint8_t {
    connected,  // A reading arrived within the stale timeout.
    stale,      // The latest reading is older than the stale timeout.
    offline,    // No reading yet, or the latest reading is older than the offline timeout.
};

struct FreshnessConfig {
    std::uint32_t stale_after_ms = 30'000;
    std::uint32_t offline_after_ms = 120'000;
};

// Keeps the latest reading and its time, and works out the link state from them.
class LatestReading {
public:
    explicit LatestReading(FreshnessConfig config) : config_(config) {}

    void update(const ChargingData& data, std::uint64_t now_ms);

    [[nodiscard]] LinkState link_state(std::uint64_t now_ms) const;

    // Age of the latest reading, or std::nullopt when there is no reading yet.
    [[nodiscard]] std::optional<std::uint64_t> age_ms(std::uint64_t now_ms) const;

    [[nodiscard]] const std::optional<ChargingData>& data() const { return data_; }

private:
    FreshnessConfig config_;
    std::optional<ChargingData> data_;
    std::uint64_t updated_at_ms_ = 0;
};

[[nodiscard]] const char* to_string(LinkState state);

}  // namespace charging_data
