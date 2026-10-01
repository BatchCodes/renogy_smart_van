// SPDX-License-Identifier: GPL-3.0-or-later
#include "cab_ui/dashboard_text.hpp"

#include <cstdio>

namespace cab_ui {

namespace {

using charging_data::LinkState;

std::string format(const char* pattern, double value) {
    char text[32];
    std::snprintf(text, sizeof(text), pattern, value);
    return text;
}

std::string format_temperature(std::int8_t celsius) {
    char text[16];
    // U+00B0 degree sign, in UTF-8. The LVGL Montserrat fonts include it.
    std::snprintf(text, sizeof(text), "%d \xC2\xB0" "C", static_cast<int>(celsius));
    return text;
}

std::string format_status(const DataSnapshot& snapshot) {
    if (!snapshot.data || !snapshot.age_ms) {
        return "Waiting for data";
    }
    if (snapshot.link_state == LinkState::offline) {
        return "Offline, last data " + format_age(*snapshot.age_ms);
    }
    return "Updated " + format_age(*snapshot.age_ms);
}

}  // namespace

std::string format_age(std::uint64_t age_ms) {
    const std::uint64_t seconds = age_ms / 1000;
    char text[24];
    if (seconds < 60) {
        std::snprintf(text, sizeof(text), "%llu s ago", static_cast<unsigned long long>(seconds));
    } else if (seconds < 3600) {
        std::snprintf(text, sizeof(text), "%llu min ago", static_cast<unsigned long long>(seconds / 60));
    } else {
        std::snprintf(text, sizeof(text), "%llu h ago", static_cast<unsigned long long>(seconds / 3600));
    }
    return text;
}

DashboardText format_dashboard(const DataSnapshot& snapshot) {
    DashboardText text;
    text.link_state = snapshot.link_state;
    text.status = format_status(snapshot);
    // Offline values are old, so the dashboard does not show them.
    if (!snapshot.data || snapshot.link_state == LinkState::offline) {
        return text;
    }

    const charging_data::ChargingData& data = *snapshot.data;
    text.has_data = true;
    text.charge_state = charging_data::to_string(data.charge_state);
    text.pv_power = format("%.0f W", data.pv_power_w);
    text.pv_voltage = format("%.1f V", data.pv_voltage_v);
    text.pv_current = format("%.1f A", data.pv_current_a);
    text.aux_voltage = format("%.2f V", data.battery_voltage_v);
    text.aux_current = format("%.1f A", data.battery_current_a);
    text.aux_soc = format("%.0f %%", data.battery_soc_percent);
    text.starter_voltage = format("%.2f V", data.alternator_voltage_v);
    text.starter_current = format("%.1f A", data.alternator_current_a);
    text.alternator_power = format("%.0f W", data.alternator_power_w);
    text.charging_power = format("%.0f W", data.battery_charging_power_w());
    text.energy_today = format("%.2f kWh", data.energy_today_wh / 1000.0);
    text.controller_temperature = format_temperature(data.controller_temperature_c);
    text.battery_temperature = format_temperature(data.battery_temperature_c);
    return text;
}

}  // namespace cab_ui
