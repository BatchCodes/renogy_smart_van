# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "main.cpp" "test_screen.cpp"
    PRIV_REQUIRES waveshare__esp32_p4_wifi6_touch_lcd_4_3
)

# ESP_LV_ADAPTER_DEFAULT_CONFIG() leaves fields to their defaults, which C++ reports
# as missing initialisers.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
