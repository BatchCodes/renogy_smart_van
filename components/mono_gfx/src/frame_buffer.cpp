// SPDX-License-Identifier: GPL-3.0-or-later
#include "mono_gfx/frame_buffer.hpp"

#include <algorithm>

namespace mono_gfx {

void FrameBuffer::clear(Color color) { bytes_.fill(color == Color::white ? 0xFF : 0x00); }

bool FrameBuffer::in_bounds(int x, int y) { return x >= 0 && x < kWidth && y >= 0 && y < kHeight; }

FrameBuffer::NativeIndex FrameBuffer::native_index(int x, int y) const {
    int native_x = 0;
    int native_y = 0;
    switch (rotation_) {
        case Rotation::clockwise_90:
            native_x = kPanelWidth - 1 - y;
            native_y = x;
            break;
        case Rotation::clockwise_270:
            native_x = y;
            native_y = kPanelHeight - 1 - x;
            break;
    }
    const auto byte =
        static_cast<std::size_t>(native_y) * kBytesPerRow + static_cast<std::size_t>(native_x / 8);
    const auto mask = static_cast<std::uint8_t>(0x80U >> (native_x % 8));
    return {byte, mask};
}

void FrameBuffer::set_pixel(int x, int y, Color color) {
    if (!in_bounds(x, y)) {
        return;
    }
    const auto index = native_index(x, y);
    if (color == Color::white) {
        bytes_[index.byte] |= index.mask;
    } else {
        bytes_[index.byte] &= static_cast<std::uint8_t>(~index.mask);
    }
}

Color FrameBuffer::pixel(int x, int y) const {
    if (!in_bounds(x, y)) {
        return Color::white;
    }
    const auto index = native_index(x, y);
    return (bytes_[index.byte] & index.mask) != 0 ? Color::white : Color::black;
}

void FrameBuffer::fill_rect(int x, int y, int width, int height, Color color) {
    const int left = std::max(x, 0);
    const int top = std::max(y, 0);
    const int right = std::min(x + width, kWidth);
    const int bottom = std::min(y + height, kHeight);
    for (int row = top; row < bottom; ++row) {
        for (int column = left; column < right; ++column) {
            set_pixel(column, row, color);
        }
    }
}

void FrameBuffer::draw_rect(int x, int y, int width, int height, Color color) {
    if (width <= 0 || height <= 0) {
        return;
    }
    draw_hline(x, y, width, color);
    draw_hline(x, y + height - 1, width, color);
    draw_vline(x, y, height, color);
    draw_vline(x + width - 1, y, height, color);
}

}  // namespace mono_gfx
