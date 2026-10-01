/* SPDX-License-Identifier: GPL-3.0-or-later */
/* LVGL configuration for the PC preview. It matches the parts of the cab_dash
 * Kconfig that change the look: 16-bit colour and the Montserrat font sizes. */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_USE_OS LV_OS_NONE
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (4 * 1024 * 1024)

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS 0

#endif
