// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "epaper_panel/panel.hpp"
#include "mono_gfx/frame_buffer.hpp"

namespace rear_eink {

// Board settings from Kconfig ("Rear e-paper display" menu).
[[nodiscard]] epaper_panel::PanelPins panel_pins();
[[nodiscard]] epaper_panel::PanelModel panel_model();
[[nodiscard]] bool detect_panel_model();
[[nodiscard]] mono_gfx::Rotation rotation();

}  // namespace rear_eink
