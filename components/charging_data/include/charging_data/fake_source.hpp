// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>

#include "charging_data/charging_data.hpp"

namespace charging_data {

struct FakeSourceConfig {
    // Length of one simulated day. Short, so a test run shows the full range of values.
    std::uint32_t day_length_ms = 180'000;
    // Each day, the source stops reporting for this long, starting at outage_start_ms.
    // This lets the display show the stale and offline states. Set the length to 0 to disable.
    std::uint32_t outage_start_ms = 150'000;
    std::uint32_t outage_length_ms = 30'000;
};

// Produces changing, plausible values with no hardware. The values depend only on
// the time, so tests can predict them.
class FakeSource final : public ChargingDataSource {
public:
    explicit FakeSource(FakeSourceConfig config) : config_(config) {}

    std::optional<ChargingData> poll(std::uint64_t now_ms) override;

    // The reading for a given time, with no outage applied.
    [[nodiscard]] ChargingData reading_at(std::uint64_t now_ms) const;

private:
    [[nodiscard]] bool in_outage(std::uint64_t now_ms) const;

    FakeSourceConfig config_;
};

}  // namespace charging_data
