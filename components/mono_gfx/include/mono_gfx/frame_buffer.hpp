// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace mono_gfx {

enum class Color : std::uint8_t { white, black };

// How the landscape drawing surface maps onto the portrait panel memory.
enum class Rotation : std::uint8_t {
    clockwise_90,
    clockwise_270,
};

// A 1 bit per pixel image for a 122 x 250 panel, stored in the panel's own memory
// layout: 250 rows of 16 bytes, most significant bit first, bit value 1 = white.
// Drawing uses landscape coordinates: x from 0 to 249, y from 0 to 121.
class FrameBuffer {
public:
    static constexpr int kPanelWidth = 122;
    static constexpr int kPanelHeight = 250;
    static constexpr int kBytesPerRow = (kPanelWidth + 7) / 8;
    static constexpr std::size_t kSizeBytes = static_cast<std::size_t>(kBytesPerRow) * kPanelHeight;

    static constexpr int kWidth = kPanelHeight;
    static constexpr int kHeight = kPanelWidth;

    explicit FrameBuffer(Rotation rotation) : rotation_(rotation) { clear(Color::white); }

    void clear(Color color);

    // Pixels outside the drawing surface are ignored.
    void set_pixel(int x, int y, Color color);
    [[nodiscard]] Color pixel(int x, int y) const;

    void fill_rect(int x, int y, int width, int height, Color color);
    void draw_rect(int x, int y, int width, int height, Color color);
    void draw_hline(int x, int y, int width, Color color) { fill_rect(x, y, width, 1, color); }
    void draw_vline(int x, int y, int height, Color color) { fill_rect(x, y, 1, height, color); }

    [[nodiscard]] std::span<const std::uint8_t> native() const { return bytes_; }

private:
    struct NativeIndex {
        std::size_t byte;
        std::uint8_t mask;
    };

    [[nodiscard]] static bool in_bounds(int x, int y);
    [[nodiscard]] NativeIndex native_index(int x, int y) const;

    Rotation rotation_;
    std::array<std::uint8_t, kSizeBytes> bytes_{};
};

}  // namespace mono_gfx
