// SPDX-License-Identifier: GPL-3.0-or-later
#include "mono_gfx/text.hpp"

namespace mono_gfx {

namespace {

const Glyph& glyph_for(const Font& font, char character) {
    if (character < font.first_char || character > font.last_char) {
        character = ' ';
    }
    return font.glyphs[character - font.first_char];
}

void draw_glyph(FrameBuffer& frame, const Font& font, const Glyph& glyph, int x, int y, Color color) {
    const int bytes_per_row = (glyph.width + 7) / 8;
    const std::uint8_t* rows = font.bitmap + glyph.bitmap_offset;
    for (int row = 0; row < font.height; ++row) {
        for (int column = 0; column < glyph.width; ++column) {
            const std::uint8_t byte = rows[row * bytes_per_row + column / 8];
            if ((byte & (0x80U >> (column % 8))) != 0) {
                frame.set_pixel(x + column, y + row, color);
            }
        }
    }
}

}  // namespace

int text_width(const Font& font, std::string_view text) {
    int width = 0;
    for (const char character : text) {
        width += glyph_for(font, character).advance;
    }
    return width;
}

int draw_text(FrameBuffer& frame, const Font& font, int x, int y, std::string_view text, Align align,
              Color color) {
    const int width = text_width(font, text);
    int cursor = x;
    if (align == Align::centre) {
        cursor = x - width / 2;
    } else if (align == Align::right) {
        cursor = x - width;
    }
    for (const char character : text) {
        const Glyph& glyph = glyph_for(font, character);
        draw_glyph(frame, font, glyph, cursor, y, color);
        cursor += glyph.advance;
    }
    return width;
}

}  // namespace mono_gfx
