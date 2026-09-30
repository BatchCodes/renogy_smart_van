# SPDX-License-Identifier: GPL-3.0-or-later
idf_component_register(
    SRCS "test_main.cpp" "test_latest_reading.cpp" "test_fake_source.cpp" "test_mono_gfx.cpp"
         "test_eink_view.cpp" "test_renogy_protocol.cpp"
    PRIV_REQUIRES unity charging_data mono_gfx eink_view renogy_protocol
    WHOLE_ARCHIVE
)

# The eink_view tests write PBM previews of each screen here.
target_compile_definitions(${COMPONENT_LIB} PRIVATE PREVIEW_DIR="${CMAKE_BINARY_DIR}/previews")
