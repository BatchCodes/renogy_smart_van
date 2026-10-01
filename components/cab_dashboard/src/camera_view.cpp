// SPDX-License-Identifier: GPL-3.0-or-later
#include "cab_dashboard/camera_view.hpp"

#include <algorithm>

namespace cab_dashboard {

namespace {

constexpr std::int32_t kButtonWidth = 180;
constexpr std::int32_t kButtonHeight = 64;
const lv_color_t kReverse = lv_color_hex(0xC0392B);

struct CallbackData {
    CameraView::Callback callback;
    void* context;
};

CallbackData dashboard_callback{};

void on_dashboard(lv_event_t* /*event*/) {
    if (dashboard_callback.callback != nullptr) {
        dashboard_callback.callback(dashboard_callback.context);
    }
}

}  // namespace

void CameraView::create(Callback on_dashboard_pressed, void* context) {
    dashboard_callback = {on_dashboard_pressed, context};

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen_, lv_color_black(), 0);
    lv_obj_remove_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);

    image_ = lv_image_create(screen_);
    lv_obj_center(image_);

    no_signal_label_ = lv_label_create(screen_);
    lv_obj_set_style_text_font(no_signal_label_, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(no_signal_label_, lv_color_white(), 0);
    lv_label_set_text(no_signal_label_, "No camera signal");
    lv_obj_center(no_signal_label_);

    reverse_badge_ = lv_label_create(screen_);
    lv_obj_set_style_text_font(reverse_badge_, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(reverse_badge_, lv_color_white(), 0);
    lv_obj_set_style_bg_color(reverse_badge_, kReverse, 0);
    lv_obj_set_style_bg_opa(reverse_badge_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(reverse_badge_, 16, 0);
    lv_obj_set_style_pad_ver(reverse_badge_, 8, 0);
    lv_obj_set_style_radius(reverse_badge_, 8, 0);
    lv_label_set_text(reverse_badge_, "REVERSE");
    lv_obj_align(reverse_badge_, LV_ALIGN_TOP_RIGHT, -12, 12);
    lv_obj_add_flag(reverse_badge_, LV_OBJ_FLAG_HIDDEN);

    dashboard_button_ = lv_button_create(screen_);
    lv_obj_set_size(dashboard_button_, kButtonWidth, kButtonHeight);
    lv_obj_align(dashboard_button_, LV_ALIGN_TOP_LEFT, 12, 12);
    lv_obj_set_style_bg_opa(dashboard_button_, LV_OPA_70, 0);
    lv_obj_add_event_cb(dashboard_button_, on_dashboard, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* label = lv_label_create(dashboard_button_);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_label_set_text(label, "Dashboard");
    lv_obj_center(label);
}

void CameraView::show_frame(const std::uint8_t* rgb565, std::uint32_t width, std::uint32_t height) {
    frame_.header.magic = LV_IMAGE_HEADER_MAGIC;
    frame_.header.cf = LV_COLOR_FORMAT_RGB565;
    frame_.header.w = width;
    frame_.header.h = height;
    frame_.header.stride = width * 2;
    frame_.data_size = width * height * 2;
    frame_.data = rgb565;

    lv_image_set_src(image_, &frame_);
    if (width != shown_width_ || height != shown_height_) {
        // Scale to fit the screen and keep the aspect ratio. 256 is 1:1 in LVGL.
        const std::int32_t screen_width = lv_display_get_horizontal_resolution(nullptr);
        const std::int32_t screen_height = lv_display_get_vertical_resolution(nullptr);
        const std::int32_t scale = std::min(screen_width * 256 / static_cast<std::int32_t>(width),
                                            screen_height * 256 / static_cast<std::int32_t>(height));
        lv_image_set_scale(image_, static_cast<std::uint32_t>(scale));
        lv_obj_center(image_);
        shown_width_ = width;
        shown_height_ = height;
    }
    lv_obj_invalidate(image_);
}

void CameraView::set_signal(bool has_signal) {
    if (has_signal) {
        lv_obj_add_flag(no_signal_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(image_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(no_signal_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(image_, LV_OBJ_FLAG_HIDDEN);
    }
}

void CameraView::set_reverse(bool reverse) {
    if (reverse) {
        lv_obj_remove_flag(reverse_badge_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(reverse_badge_, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace cab_dashboard
