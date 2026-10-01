# SPDX-License-Identifier: GPL-3.0-or-later
#
# Renders the cab dashboard on a PC and writes one image per page.
#   cmake -S tools/lvgl_preview -B tools/lvgl_preview/build && cmake --build tools/lvgl_preview/build
#   tools/lvgl_preview/build/lvgl_preview tools/lvgl_preview/build/previews
cmake_minimum_required(VERSION 3.22)
project(lvgl_preview C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)

# Use the LVGL that the cab_dash build downloaded, so the preview matches the firmware.
# Without it, download the same version.
set(LVGL_LOCAL_DIR "${REPO_DIR}/firmware/cab_dash/managed_components/lvgl__lvgl")
if(EXISTS "${LVGL_LOCAL_DIR}/CMakeLists.txt")
    add_subdirectory("${LVGL_LOCAL_DIR}" lvgl)
else()
    include(FetchContent)
    FetchContent_Declare(lvgl GIT_REPOSITORY https://github.com/lvgl/lvgl.git GIT_TAG v9.5.0 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(lvgl)
endif()

add_subdirectory("${REPO_DIR}/components/cab_dashboard" cab_dashboard)
target_link_libraries(cab_dashboard PUBLIC lvgl)
target_include_directories(cab_dashboard PUBLIC
    "${REPO_DIR}/components/cab_ui/include"
    "${REPO_DIR}/components/charging_data/include")

add_executable(lvgl_preview
    preview.cpp
    "${REPO_DIR}/components/cab_ui/src/dashboard_text.cpp"
    "${REPO_DIR}/components/cab_ui/src/power_history.cpp"
    "${REPO_DIR}/components/charging_data/src/fake_source.cpp"
    "${REPO_DIR}/components/charging_data/src/latest_reading.cpp")
target_link_libraries(lvgl_preview PRIVATE cab_dashboard lvgl)
