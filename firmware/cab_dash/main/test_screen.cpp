// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_screen.hpp"

#include <array>
#include <cstdint>
#include <cstdio>

#include "display.hpp"
#include "esp_log.h"
#include "lvgl.h"

namespace cab_dash {

namespace {

constexpr const char* kTag = "test_screen";

lv_obj_t* tap_label = nullptr;
unsigned tap_count = 0;

void on_tap(lv_event_t* event) {
    ++tap_count;
    char text[32];
    std::snprintf(text, sizeof(text), "Taps: %u", tap_count);
    lv_label_set_text(tap_label, text);

    lv_indev_t* input = lv_indev_active();
    lv_point_t point{};
    if (input != nullptr) {
        lv_indev_get_point(input, &point);
    }
    ESP_LOGI(kTag, "Tap %u at (%ld, %ld)", tap_count, static_cast<long>(point.x),
             static_cast<long>(point.y));
    (void)event;
}

}  // namespace

void show_test_screen() {
    const DisplayLock lock;
    if (!lock.locked()) {
        ESP_LOGE(kTag, "Display lock failed");
        return;
    }

    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    lv_obj_t* title = lv_label_create(screen);
    char text[64];
    std::snprintf(text, sizeof(text), "Cab dashboard test  %ld x %ld",
                  static_cast<long>(lv_display_get_horizontal_resolution(nullptr)),
                  static_cast<long>(lv_display_get_vertical_resolution(nullptr)));
    lv_label_set_text(title, text);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t* button = lv_button_create(screen);
    lv_obj_set_size(button, 240, 120);
    lv_obj_center(button);
    lv_obj_add_event_cb(button, on_tap, LV_EVENT_CLICKED, nullptr);
    tap_label = lv_label_create(button);
    lv_label_set_text(tap_label, "Tap me");
    lv_obj_center(tap_label);

    // Colour bar along the bottom edge, to check the colour order of the panel.
    constexpr std::array<std::uint32_t, 4> kColours{0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF};
    const lv_coord_t width = lv_display_get_horizontal_resolution(nullptr) / kColours.size();
    for (std::size_t index = 0; index < kColours.size(); ++index) {
        lv_obj_t* bar = lv_obj_create(screen);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, width, 40);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(kColours[index]), 0);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, static_cast<lv_coord_t>(index) * width, 0);
    }
}

}  // namespace cab_dash
