# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "test_main.cpp" "test_latest_reading.cpp" "test_fake_source.cpp" "test_mono_gfx.cpp"
    PRIV_REQUIRES unity charging_data mono_gfx
    WHOLE_ARCHIVE
)
