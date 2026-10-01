// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "cab_dashboard/camera_view.hpp"

namespace cab_dash {

// Connects the camera to the camera view: an LVGL timer shows each new frame and the
// signal state. Call create() once, in the LVGL task or under the LVGL lock.
class CameraScreen {
public:
    void create(cab_dashboard::CameraView::Callback on_dashboard_pressed, void* context);

    [[nodiscard]] cab_dashboard::CameraView& view() { return view_; }

    // Called by the LVGL timer.
    void refresh();

private:
    cab_dashboard::CameraView view_;
    std::uint32_t shown_sequence_ = 0;
};

}  // namespace cab_dash
