# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "src/bt2_ble_source.cpp"
    INCLUDE_DIRS "include"
    REQUIRES charging_data
    PRIV_REQUIRES bt nvs_flash renogy_protocol
)
