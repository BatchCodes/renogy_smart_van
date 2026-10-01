// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "data_hub.hpp"

namespace cab_dash {

// Builds the dashboard and starts an LVGL timer that redraws it from the data hub and
// controls the backlight timeout.
void start_dashboard(DataHub& hub);

}  // namespace cab_dash
