// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "lvgl.h"

namespace cab_dashboard {

// The rear camera screen: the camera image scaled to fit, a "Dashboard" button, a
// REVERSE badge and a message when there is no signal. It is a separate LVGL screen.
// All calls must run in the LVGL task or under the LVGL lock.
class CameraView {
public:
    using Callback = void (*)(void* context);

    void create(Callback on_dashboard_pressed, void* context);

    [[nodiscard]] lv_obj_t* screen() const { return screen_; }

    // Shows a new frame. The pixels must stay valid until the next call.
    void show_frame(const std::uint8_t* rgb565, std::uint32_t width, std::uint32_t height);

    void set_signal(bool has_signal);
    void set_reverse(bool reverse);

private:
    lv_obj_t* screen_ = nullptr;
    lv_obj_t* image_ = nullptr;
    lv_obj_t* no_signal_label_ = nullptr;
    lv_obj_t* reverse_badge_ = nullptr;
    lv_obj_t* dashboard_button_ = nullptr;
    lv_image_dsc_t frame_{};
    std::uint32_t shown_width_ = 0;
    std::uint32_t shown_height_ = 0;
};

}  // namespace cab_dashboard
