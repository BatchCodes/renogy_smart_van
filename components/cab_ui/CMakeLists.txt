# SPDX-License-Identifier: GPL-3.0-or-later
# Pure C++ component. It must not depend on ESP-IDF or LVGL, so the host tests can build it.
idf_component_register(
    SRCS "src/dashboard_text.cpp" "src/power_history.cpp"
    INCLUDE_DIRS "include"
    REQUIRES charging_data
)
