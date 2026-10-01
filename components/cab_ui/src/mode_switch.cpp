// SPDX-License-Identifier: GPL-3.0-or-later
#include "cab_ui/mode_switch.hpp"

namespace cab_ui {

void ModeSwitch::set_reverse(bool reverse, std::uint64_t now_ms) {
    if (reverse == reverse_) {
        return;
    }
    reverse_ = reverse;
    if (reverse) {
        reverse_ended_ms_.reset();
    } else {
        reverse_ended_ms_ = now_ms;
    }
}

bool ModeSwitch::request(Screen screen) {
    if (reverse_) {
        return false;
    }
    // A touch during the hold time ends the hold.
    reverse_ended_ms_.reset();
    manual_ = screen;
    return true;
}

Screen ModeSwitch::screen(std::uint64_t now_ms) const {
    if (reverse_) {
        return Screen::camera;
    }
    if (reverse_ended_ms_ && now_ms - *reverse_ended_ms_ < config_.camera_hold_ms) {
        return Screen::camera;
    }
    return manual_;
}

}  // namespace cab_ui
