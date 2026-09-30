// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>

#include "charging_data/latest_reading.hpp"
#include "mono_gfx/frame_buffer.hpp"

namespace eink_view {

// Draws the rear display layout for the latest reading. The link state selects the
// layout: the charging values when connected or stale, and an offline screen when
// offline. A stale screen shows the data age.
void render(mono_gfx::FrameBuffer& frame, const charging_data::LatestReading& reading,
            std::uint64_t now_ms);

// "12 s ago", "5 min ago", "3 h ago".
[[nodiscard]] std::string format_age(std::uint64_t age_ms);

}  // namespace eink_view
