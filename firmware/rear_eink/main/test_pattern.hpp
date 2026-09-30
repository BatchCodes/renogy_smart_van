// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "epaper_panel/panel.hpp"

namespace rear_eink {

// Draws a fixed pattern with a counter, refreshes at the configured interval and logs
// each refresh time. Does not return.
[[noreturn]] void run_test_pattern(epaper_panel::PanelDevice& device);

}  // namespace rear_eink
