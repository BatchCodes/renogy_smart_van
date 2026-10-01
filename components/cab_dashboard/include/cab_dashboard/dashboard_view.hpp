// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "cab_ui/dashboard_text.hpp"
#include "cab_ui/power_history.hpp"
#include "lvgl.h"

namespace cab_dashboard {

enum class Page : std::uint8_t { solar, batteries, temperatures, energy };

// The cab dashboard: a status bar, four pages with tabs at the bottom, and an
// offline message. Designed for an 800 x 480 landscape screen. All calls must run
// in the LVGL task or under the LVGL lock.
class DashboardView {
public:
    using Callback = void (*)(void* context);

    // Builds the widgets on the screen. on_camera_pressed runs when the user taps the
    // "Camera" button. Without a callback, the button is hidden.
    void create(lv_obj_t* screen, Callback on_camera_pressed = nullptr, void* context = nullptr);

    [[nodiscard]] lv_obj_t* screen() const { return screen_; }

    void update(const cab_ui::DashboardText& text, const cab_ui::PowerHistory& history);

    void show_page(Page page);

private:
    void create_status_bar(lv_obj_t* screen, Callback on_camera_pressed, void* context);
    void create_solar_page(lv_obj_t* page);
    void create_batteries_page(lv_obj_t* page);
    void create_temperatures_page(lv_obj_t* page);
    void create_energy_page(lv_obj_t* page);
    void update_chart(const cab_ui::PowerHistory& history);

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* status_bar_ = nullptr;
    lv_obj_t* camera_button_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* state_label_ = nullptr;
    lv_obj_t* tabview_ = nullptr;
    lv_obj_t* offline_panel_ = nullptr;
    lv_obj_t* offline_label_ = nullptr;

    lv_obj_t* pv_power_ = nullptr;
    lv_obj_t* charge_mode_ = nullptr;
    lv_obj_t* pv_voltage_ = nullptr;
    lv_obj_t* pv_current_ = nullptr;
    lv_obj_t* charging_power_ = nullptr;

    lv_obj_t* aux_voltage_ = nullptr;
    lv_obj_t* aux_current_ = nullptr;
    lv_obj_t* aux_soc_ = nullptr;
    lv_obj_t* starter_voltage_ = nullptr;
    lv_obj_t* starter_current_ = nullptr;
    lv_obj_t* alternator_power_ = nullptr;

    lv_obj_t* controller_temperature_ = nullptr;
    lv_obj_t* battery_temperature_ = nullptr;

    lv_obj_t* energy_today_ = nullptr;
    lv_obj_t* chart_ = nullptr;
    lv_chart_series_t* chart_series_ = nullptr;
    lv_obj_t* chart_max_label_ = nullptr;
};

}  // namespace cab_dashboard
