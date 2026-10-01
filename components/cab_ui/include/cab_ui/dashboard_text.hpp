// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "charging_data/charging_data.hpp"
#include "charging_data/latest_reading.hpp"

namespace cab_ui {

// What the dashboard needs to draw one frame.
struct DataSnapshot {
    std::optional<charging_data::ChargingData> data;
    charging_data::LinkState link_state = charging_data::LinkState::offline;
    std::optional<std::uint64_t> age_ms;
};

// Every text on the dashboard, formatted with units. Empty when there is no data.
struct DashboardText {
    charging_data::LinkState link_state = charging_data::LinkState::offline;
    bool has_data = false;
    std::string status;  // "Updated 3 s ago", "Waiting for data", "Offline, last data 5 min ago"
    std::string charge_state;

    std::string pv_power;
    std::string pv_voltage;
    std::string pv_current;

    std::string aux_voltage;
    std::string aux_current;
    std::string aux_soc;

    std::string starter_voltage;
    std::string starter_current;
    std::string alternator_power;

    std::string charging_power;
    std::string energy_today;

    std::string controller_temperature;
    std::string battery_temperature;
};

[[nodiscard]] DashboardText format_dashboard(const DataSnapshot& snapshot);

// "12 s ago", "5 min ago", "3 h ago".
[[nodiscard]] std::string format_age(std::uint64_t age_ms);

}  // namespace cab_ui
