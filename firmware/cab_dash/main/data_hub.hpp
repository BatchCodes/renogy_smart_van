// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <mutex>
#include <optional>

#include "charging_data/charging_data.hpp"
#include "charging_data/latest_reading.hpp"

namespace cab_dash {

// What the user interface needs to draw one frame.
struct DataSnapshot {
    std::optional<charging_data::ChargingData> data;
    charging_data::LinkState link_state = charging_data::LinkState::offline;
    std::optional<std::uint64_t> age_ms;
};

// Polls a source in its own task and keeps the latest reading. The user interface
// takes a snapshot from the LVGL task. Only the snapshot crosses tasks, under a mutex.
class DataHub {
public:
    DataHub(charging_data::ChargingDataSource& source, charging_data::FreshnessConfig freshness)
        : source_(source), reading_(freshness) {}

    // Starts the poll task. Call once.
    void start(std::uint32_t poll_interval_ms);

    [[nodiscard]] DataSnapshot snapshot(std::uint64_t now_ms);

    // Called by the poll task.
    void poll_once(std::uint64_t now_ms);

    [[nodiscard]] std::uint32_t poll_interval_ms() const { return poll_interval_ms_; }

private:
    charging_data::ChargingDataSource& source_;
    std::mutex mutex_;
    charging_data::LatestReading reading_;
    std::uint32_t poll_interval_ms_ = 1000;
};

[[nodiscard]] std::uint64_t now_ms();

}  // namespace cab_dash
