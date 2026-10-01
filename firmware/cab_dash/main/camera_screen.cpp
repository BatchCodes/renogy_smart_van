// SPDX-License-Identifier: GPL-3.0-or-later
#include "camera_screen.hpp"

#include "camera.hpp"
#include "data_hub.hpp"

namespace cab_dash {

namespace {

// About 30 frames per second. The timer does nothing while no new frame is ready.
constexpr std::uint32_t kRefreshIntervalMs = 33;

void on_timer(lv_timer_t* timer) { static_cast<CameraScreen*>(lv_timer_get_user_data(timer))->refresh(); }

}  // namespace

void CameraScreen::create(cab_dashboard::CameraView::Callback on_dashboard_pressed, void* context) {
    view_.create(on_dashboard_pressed, context);
    view_.set_signal(false);
    lv_timer_create(on_timer, kRefreshIntervalMs, this);
}

void CameraScreen::refresh() {
    if (lv_screen_active() != view_.screen()) {
        return;
    }
    Camera& camera = Camera::instance();
    const CameraFrame frame = camera.latest();
    if (frame.pixels != nullptr && frame.sequence != shown_sequence_) {
        camera.displaying(frame.sequence);
        view_.show_frame(frame.pixels, frame.width, frame.height);
        shown_sequence_ = frame.sequence;
    }
    view_.set_signal(camera.has_signal(now_ms()));
}

}  // namespace cab_dash
