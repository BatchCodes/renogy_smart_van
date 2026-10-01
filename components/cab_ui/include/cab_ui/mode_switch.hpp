// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>

namespace cab_ui {

enum class Screen : std::uint8_t { dashboard, camera };

struct ModeSwitchConfig {
    // After reverse ends, the camera stays on screen for this time. This keeps the
    // camera through a quick change between reverse and first gear while parking.
    std::uint32_t camera_hold_ms = 2000;
};

// Decides which screen the cab unit shows.
// - Reverse selects the camera and overrides the manual selection.
// - When reverse ends, the camera stays for the hold time, then the manual selection
//   returns.
// - A touch request selects a screen, except while the van is in reverse.
class ModeSwitch {
public:
    explicit ModeSwitch(ModeSwitchConfig config) : config_(config) {}

    // The debounced reverse input.
    void set_reverse(bool reverse, std::uint64_t now_ms);

    // A touch request. Returns false if reverse overrides it.
    bool request(Screen screen);

    [[nodiscard]] Screen screen(std::uint64_t now_ms) const;
    [[nodiscard]] bool reverse() const { return reverse_; }

private:
    ModeSwitchConfig config_;
    Screen manual_ = Screen::dashboard;
    bool reverse_ = false;
    std::optional<std::uint64_t> reverse_ended_ms_;
};

}  // namespace cab_ui
