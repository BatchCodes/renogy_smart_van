// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

namespace mono_gfx {

// One character of a bitmap font. The bitmap rows are packed, most significant
// bit first, each row starting on a new byte.
struct Glyph {
    std::uint32_t bitmap_offset;
    std::uint8_t width;
    std::uint8_t advance;
};

// A proportional bitmap font for the printable ASCII characters. All glyphs share
// the same height, so a text line has a fixed height.
struct Font {
    const std::uint8_t* bitmap;
    const Glyph* glyphs;
    char first_char;
    char last_char;
    std::uint8_t height;
    std::uint8_t baseline;  // Rows from the top of a glyph to the baseline.
};

extern const Font kFontSmall;   // About 12 px
extern const Font kFontMedium;  // About 18 px
extern const Font kFontLarge;   // About 40 px, bold

}  // namespace mono_gfx
