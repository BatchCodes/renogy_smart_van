# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "main.cpp"
    PRIV_REQUIRES renogy_ble charging_data esp_timer
)
