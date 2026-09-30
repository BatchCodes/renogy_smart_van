// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string_view>

#include "mono_gfx/font.hpp"
#include "mono_gfx/frame_buffer.hpp"

namespace mono_gfx {

enum class Align : std::uint8_t { left, centre, right };

// Width in pixels of the text in the given font. Unknown characters count as a space.
[[nodiscard]] int text_width(const Font& font, std::string_view text);

// Draws the text with its top edge at y. x is the left edge, the centre or the right
// edge, as set by align. Returns the width of the text.
int draw_text(FrameBuffer& frame, const Font& font, int x, int y, std::string_view text,
              Align align = Align::left, Color color = Color::black);

}  // namespace mono_gfx
