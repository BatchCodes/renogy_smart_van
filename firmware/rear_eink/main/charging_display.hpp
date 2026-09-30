// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "charging_data/charging_data.hpp"
#include "epaper_panel/panel.hpp"

namespace rear_eink {

// Polls the source, draws the charging layout and refreshes the panel when the image
// changes. Does not return.
[[noreturn]] void run_charging_display(epaper_panel::PanelDevice& device,
                                       charging_data::ChargingDataSource& source);

}  // namespace rear_eink
