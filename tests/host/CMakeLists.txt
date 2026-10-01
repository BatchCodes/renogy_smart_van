# SPDX-License-Identifier: GPL-3.0-or-later
# Host tests for the pure C++ components. Build and run them on the ESP-IDF linux target.
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components/charging_data"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/mono_gfx"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/eink_view"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/renogy_protocol"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/cab_ui"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(COMPONENTS main)
project(host_tests)
