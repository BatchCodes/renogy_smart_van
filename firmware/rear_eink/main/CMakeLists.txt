# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "main.cpp" "board.cpp" "test_pattern.cpp"
    PRIV_REQUIRES esp_driver_gpio esp_timer epaper_panel mono_gfx
)
