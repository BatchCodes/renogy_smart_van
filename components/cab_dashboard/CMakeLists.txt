# SPDX-License-Identifier: GPL-3.0-or-later
# LVGL dashboard view. It uses LVGL and cab_ui, and no ESP-IDF headers, so
# tools/lvgl_preview can build it on a PC.
if(ESP_PLATFORM)
    idf_component_register(
        SRCS "src/dashboard_view.cpp"
        INCLUDE_DIRS "include"
        REQUIRES cab_ui lvgl__lvgl
    )
else()
    add_library(cab_dashboard STATIC "${CMAKE_CURRENT_LIST_DIR}/src/dashboard_view.cpp")
    target_include_directories(cab_dashboard PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
endif()
