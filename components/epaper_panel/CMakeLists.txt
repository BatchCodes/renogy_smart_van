# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "src/panel_io.cpp" "src/ssd1680_panel.cpp" "src/ssd1682_panel.cpp" "src/jd79656_panel.cpp"
         "src/panel_factory.cpp"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_driver_spi esp_driver_gpio
    PRIV_REQUIRES esp_timer
)
