// SPDX-License-Identifier: GPL-3.0-or-later
#include "cab_dashboard/dashboard_view.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace cab_dashboard {

namespace {

using charging_data::LinkState;

constexpr std::int32_t kStatusBarHeight = 44;
constexpr std::int32_t kTabBarHeight = 64;
constexpr std::int32_t kChartMinimumW = 100;

const lv_color_t kBackground = lv_color_hex(0x101418);
const lv_color_t kCard = lv_color_hex(0x1C232B);
const lv_color_t kText = lv_color_hex(0xF2F4F7);
const lv_color_t kMuted = lv_color_hex(0x98A2B3);
const lv_color_t kSolar = lv_color_hex(0xF5B041);
const lv_color_t kStale = lv_color_hex(0xB9770E);
const lv_color_t kOffline = lv_color_hex(0x922B21);

struct CameraCallback {
    DashboardView::Callback callback;
    void* context;
};

CameraCallback camera_callback{};

void on_camera(lv_event_t* /*event*/) {
    if (camera_callback.callback != nullptr) {
        camera_callback.callback(camera_callback.context);
    }
}

lv_obj_t* make_label(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, const char* text = "") {
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_label_set_text(label, text);
    return label;
}

// A rounded panel that lays out its children in a column.
lv_obj_t* make_card(lv_obj_t* parent, const char* title) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_style_bg_color(card, kCard, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_pad_row(card, 4, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    make_label(card, &lv_font_montserrat_20, kMuted, title);
    return card;
}

// A row inside a card: a small caption on the left, a value on the right.
lv_obj_t* make_value_row(lv_obj_t* card, const char* caption, const lv_font_t* font) {
    lv_obj_t* row = lv_obj_create(card);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    make_label(row, &lv_font_montserrat_20, kMuted, caption);
    return make_label(row, font, kText, "--");
}

void set_text(lv_obj_t* label, const std::string& text) {
    lv_label_set_text(label, text.empty() ? "--" : text.c_str());
}

void style_page(lv_obj_t* page) {
    lv_obj_set_style_bg_color(page, kBackground, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(page, 12, 0);
    lv_obj_set_style_pad_column(page, 12, 0);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
}

}  // namespace

void DashboardView::create(lv_obj_t* screen, Callback on_camera_pressed, void* context) {
    screen_ = screen;
    lv_obj_set_style_bg_color(screen, kBackground, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    create_status_bar(screen, on_camera_pressed, context);

    tabview_ = lv_tabview_create(screen);
    lv_obj_set_size(tabview_, lv_pct(100), lv_display_get_vertical_resolution(nullptr) - kStatusBarHeight);
    lv_obj_align(tabview_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(tabview_, kBackground, 0);
    lv_tabview_set_tab_bar_position(tabview_, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(tabview_, kTabBarHeight);

    lv_obj_t* tab_bar = lv_tabview_get_tab_bar(tabview_);
    lv_obj_set_style_bg_color(tab_bar, kCard, 0);
    lv_obj_set_style_text_font(tab_bar, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(tab_bar, kText, 0);

    create_solar_page(lv_tabview_add_tab(tabview_, "Solar"));
    create_batteries_page(lv_tabview_add_tab(tabview_, "Batteries"));
    create_temperatures_page(lv_tabview_add_tab(tabview_, "Temps"));
    create_energy_page(lv_tabview_add_tab(tabview_, "Energy"));

    // Covers the pages, but not the status bar and the tabs, when there is no current
    // data. It is created last, so it is on top.
    const std::int32_t content_height =
        lv_display_get_vertical_resolution(nullptr) - kStatusBarHeight - kTabBarHeight;
    offline_panel_ = lv_obj_create(screen);
    lv_obj_set_size(offline_panel_, lv_pct(100), content_height);
    lv_obj_align(offline_panel_, LV_ALIGN_TOP_MID, 0, kStatusBarHeight);
    lv_obj_set_style_radius(offline_panel_, 0, 0);
    lv_obj_set_style_bg_color(offline_panel_, kBackground, 0);
    lv_obj_set_style_border_width(offline_panel_, 0, 0);
    lv_obj_remove_flag(offline_panel_, LV_OBJ_FLAG_SCROLLABLE);
    offline_label_ = make_label(offline_panel_, &lv_font_montserrat_28, kText, "Renogy offline");
    lv_obj_center(offline_label_);
}

void DashboardView::create_status_bar(lv_obj_t* screen, Callback on_camera_pressed, void* context) {
    status_bar_ = lv_obj_create(screen);
    lv_obj_remove_style_all(status_bar_);
    lv_obj_set_size(status_bar_, lv_pct(100), kStatusBarHeight);
    lv_obj_set_style_bg_opa(status_bar_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(status_bar_, kCard, 0);
    lv_obj_set_style_pad_hor(status_bar_, 16, 0);
    lv_obj_align(status_bar_, LV_ALIGN_TOP_MID, 0, 0);

    state_label_ = make_label(status_bar_, &lv_font_montserrat_20, kSolar, "--");
    lv_obj_align(state_label_, LV_ALIGN_LEFT_MID, 0, 0);
    status_label_ = make_label(status_bar_, &lv_font_montserrat_20, kText, "Waiting for data");
    lv_obj_align(status_label_, LV_ALIGN_RIGHT_MID, 0, 0);

    if (on_camera_pressed == nullptr) {
        return;
    }
    camera_callback = {on_camera_pressed, context};
    camera_button_ = lv_button_create(status_bar_);
    lv_obj_set_size(camera_button_, 160, kStatusBarHeight - 6);
    lv_obj_align(camera_button_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(camera_button_, on_camera, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* label = make_label(camera_button_, &lv_font_montserrat_20, kText, "Camera");
    lv_obj_center(label);
}

void DashboardView::create_solar_page(lv_obj_t* page) {
    style_page(page);

    lv_obj_t* power_card = make_card(page, "SOLAR");
    lv_obj_set_size(power_card, lv_pct(58), lv_pct(100));
    pv_power_ = make_label(power_card, &lv_font_montserrat_48, kSolar, "--");
    charge_mode_ = make_value_row(power_card, "Mode", &lv_font_montserrat_28);
    lv_obj_t* spacer = lv_obj_create(power_card);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_flex_grow(spacer, 1);
    charging_power_ = make_value_row(power_card, "Into AUX battery", &lv_font_montserrat_28);

    lv_obj_t* panel_card = make_card(page, "PANELS");
    lv_obj_set_flex_grow(panel_card, 1);
    lv_obj_set_height(panel_card, lv_pct(100));
    pv_voltage_ = make_value_row(panel_card, "Voltage", &lv_font_montserrat_48);
    pv_current_ = make_value_row(panel_card, "Current", &lv_font_montserrat_48);
}

void DashboardView::create_batteries_page(lv_obj_t* page) {
    style_page(page);

    lv_obj_t* aux_card = make_card(page, "AUX BATTERY");
    lv_obj_set_flex_grow(aux_card, 1);
    lv_obj_set_height(aux_card, lv_pct(100));
    aux_voltage_ = make_value_row(aux_card, "Voltage", &lv_font_montserrat_48);
    aux_current_ = make_value_row(aux_card, "Charge", &lv_font_montserrat_28);
    aux_soc_ = make_value_row(aux_card, "State of charge", &lv_font_montserrat_28);

    lv_obj_t* starter_card = make_card(page, "STARTER BATTERY");
    lv_obj_set_flex_grow(starter_card, 1);
    lv_obj_set_height(starter_card, lv_pct(100));
    starter_voltage_ = make_value_row(starter_card, "Voltage", &lv_font_montserrat_48);
    starter_current_ = make_value_row(starter_card, "Alternator", &lv_font_montserrat_28);
    alternator_power_ = make_value_row(starter_card, "Power", &lv_font_montserrat_28);
}

void DashboardView::create_temperatures_page(lv_obj_t* page) {
    style_page(page);

    lv_obj_t* controller_card = make_card(page, "CHARGER");
    lv_obj_set_flex_grow(controller_card, 1);
    lv_obj_set_height(controller_card, lv_pct(100));
    controller_temperature_ = make_label(controller_card, &lv_font_montserrat_48, kText, "--");

    lv_obj_t* battery_card = make_card(page, "AUX BATTERY");
    lv_obj_set_flex_grow(battery_card, 1);
    lv_obj_set_height(battery_card, lv_pct(100));
    battery_temperature_ = make_label(battery_card, &lv_font_montserrat_48, kText, "--");
}

void DashboardView::create_energy_page(lv_obj_t* page) {
    style_page(page);

    lv_obj_t* today_card = make_card(page, "TODAY");
    lv_obj_set_size(today_card, lv_pct(30), lv_pct(100));
    energy_today_ = make_label(today_card, &lv_font_montserrat_28, kSolar, "--");

    lv_obj_t* chart_card = make_card(page, "SOLAR POWER, LAST HOUR");
    lv_obj_set_flex_grow(chart_card, 1);
    lv_obj_set_height(chart_card, lv_pct(100));
    chart_max_label_ = make_label(chart_card, &lv_font_montserrat_14, kMuted, "");

    chart_ = lv_chart_create(chart_card);
    lv_obj_set_width(chart_, lv_pct(100));
    lv_obj_set_flex_grow(chart_, 1);
    lv_chart_set_type(chart_, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(chart_, 4, 0);
    lv_obj_set_style_bg_color(chart_, kCard, 0);
    lv_obj_set_style_border_width(chart_, 0, 0);
    lv_obj_set_style_line_color(chart_, lv_color_hex(0x344054), LV_PART_MAIN);
    lv_obj_set_style_size(chart_, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(chart_, 3, LV_PART_ITEMS);
    chart_series_ = lv_chart_add_series(chart_, kSolar, LV_CHART_AXIS_PRIMARY_Y);
}

void DashboardView::update_chart(const cab_ui::PowerHistory& history) {
    const std::vector<float> values = history.values();
    lv_chart_set_point_count(chart_, static_cast<std::uint32_t>(history.capacity()));
    std::vector<std::int32_t> points(history.capacity(), LV_CHART_POINT_NONE);
    // Newest sample at the right edge.
    const std::size_t offset = history.capacity() - values.size();
    for (std::size_t index = 0; index < values.size(); ++index) {
        points[offset + index] = static_cast<std::int32_t>(std::lround(values[index]));
    }
    const auto top = std::max<std::int32_t>(kChartMinimumW, static_cast<std::int32_t>(std::ceil(history.max_value() / 50.0F) * 50));
    lv_chart_set_axis_range(chart_, LV_CHART_AXIS_PRIMARY_Y, 0, top);
    lv_chart_set_series_values(chart_, chart_series_, points.data(), points.size());

    char text[32];
    std::snprintf(text, sizeof(text), "0 to %ld W", static_cast<long>(top));
    lv_label_set_text(chart_max_label_, text);
}

void DashboardView::update(const cab_ui::DashboardText& text, const cab_ui::PowerHistory& history) {
    lv_label_set_text(status_label_, text.status.c_str());
    set_text(state_label_, text.charge_state);
    switch (text.link_state) {
        case LinkState::connected:
            lv_obj_set_style_bg_color(status_bar_, kCard, 0);
            break;
        case LinkState::stale:
            lv_obj_set_style_bg_color(status_bar_, kStale, 0);
            break;
        case LinkState::offline:
            lv_obj_set_style_bg_color(status_bar_, kOffline, 0);
            break;
    }

    if (text.has_data) {
        lv_obj_add_flag(offline_panel_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(offline_panel_, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(offline_label_, text.status == "Waiting for data" ? "Waiting for the BT-2" : "Renogy offline");
    }

    set_text(pv_power_, text.pv_power);
    set_text(charge_mode_, text.charge_state);
    set_text(pv_voltage_, text.pv_voltage);
    set_text(pv_current_, text.pv_current);
    set_text(charging_power_, text.charging_power);
    set_text(aux_voltage_, text.aux_voltage);
    set_text(aux_current_, text.aux_current);
    set_text(aux_soc_, text.aux_soc);
    set_text(starter_voltage_, text.starter_voltage);
    set_text(starter_current_, text.starter_current);
    set_text(alternator_power_, text.alternator_power);
    set_text(controller_temperature_, text.controller_temperature);
    set_text(battery_temperature_, text.battery_temperature);
    set_text(energy_today_, text.energy_today);
    update_chart(history);
}

void DashboardView::show_page(Page page) {
    lv_tabview_set_active(tabview_, static_cast<std::uint32_t>(page), LV_ANIM_OFF);
}

}  // namespace cab_dashboard
