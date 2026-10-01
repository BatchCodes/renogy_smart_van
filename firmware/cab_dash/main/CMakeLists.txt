# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "main.cpp" "test_screen.cpp" "ble.cpp" "data_hub.cpp" "dashboard.cpp" "camera.cpp" "camera_screen.cpp" "reverse_input.cpp"
    PRIV_REQUIRES waveshare__esp32_p4_wifi6_touch_lcd_4_3 espressif__esp_hosted renogy_ble charging_data esp_timer cab_ui cab_dashboard espressif__usb_host_uvc esp_driver_jpeg esp_driver_gpio
)

# ESP_LV_ADAPTER_DEFAULT_CONFIG() leaves fields to their defaults, which C++ reports
# as missing initialisers.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
