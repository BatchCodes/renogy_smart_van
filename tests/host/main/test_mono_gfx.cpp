// SPDX-License-Identifier: GPL-3.0-or-later
#include <memory>

#include "mono_gfx/frame_buffer.hpp"
#include "mono_gfx/text.hpp"
#include "tests.hpp"
#include "unity.h"

using mono_gfx::Align;
using mono_gfx::Color;
using mono_gfx::FrameBuffer;
using mono_gfx::Rotation;

namespace {

int count_black(const FrameBuffer& frame) {
    int count = 0;
    for (int y = 0; y < FrameBuffer::kHeight; ++y) {
        for (int x = 0; x < FrameBuffer::kWidth; ++x) {
            count += frame.pixel(x, y) == Color::black ? 1 : 0;
        }
    }
    return count;
}

void test_new_frame_is_white() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    TEST_ASSERT_EQUAL_size_t(4000, frame->native().size());
    for (const auto byte : frame->native()) {
        TEST_ASSERT_EQUAL_HEX8(0xFF, byte);
    }
}

void test_rotation_270_maps_top_left_to_last_row() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    frame->set_pixel(0, 0, Color::black);
    // Landscape (0, 0) is native x 0, native y 249: the first bit of the last row.
    TEST_ASSERT_EQUAL_HEX8(0x7F, frame->native()[249 * 16]);
    TEST_ASSERT_TRUE(frame->pixel(0, 0) == Color::black);
}

void test_rotation_90_maps_top_left_to_first_row() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_90);
    frame->set_pixel(0, 0, Color::black);
    // Landscape (0, 0) is native x 121, native y 0: byte 15, bit 6 from the top.
    TEST_ASSERT_EQUAL_HEX8(0xBF, frame->native()[15]);
}

void test_out_of_bounds_pixels_are_ignored() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    frame->set_pixel(-1, 0, Color::black);
    frame->set_pixel(FrameBuffer::kWidth, 0, Color::black);
    frame->set_pixel(0, FrameBuffer::kHeight, Color::black);
    TEST_ASSERT_EQUAL_INT(0, count_black(*frame));
    TEST_ASSERT_TRUE(frame->pixel(-5, -5) == Color::white);
}

void test_fill_rect_is_clipped() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    frame->fill_rect(245, 118, 20, 20, Color::black);
    TEST_ASSERT_EQUAL_INT(5 * 4, count_black(*frame));
}

void test_draw_rect_outline() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_90);
    frame->draw_rect(10, 10, 5, 4, Color::black);
    TEST_ASSERT_EQUAL_INT(14, count_black(*frame));
    TEST_ASSERT_TRUE(frame->pixel(12, 11) == Color::white);
}

void test_text_width_adds_advances() {
    const auto& font = mono_gfx::kFontSmall;
    const int width_a = mono_gfx::text_width(font, "A");
    TEST_ASSERT_TRUE(width_a > 0);
    TEST_ASSERT_EQUAL_INT(width_a * 3, mono_gfx::text_width(font, "AAA"));
    TEST_ASSERT_EQUAL_INT(0, mono_gfx::text_width(font, ""));
    // An unknown character counts as a space.
    TEST_ASSERT_EQUAL_INT(mono_gfx::text_width(font, " "), mono_gfx::text_width(font, "\x01"));
}

void test_draw_text_alignment() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    const auto& font = mono_gfx::kFontMedium;
    const int width = mono_gfx::draw_text(*frame, font, 200, 10, "88", Align::right);
    TEST_ASSERT_EQUAL_INT(mono_gfx::text_width(font, "88"), width);
    TEST_ASSERT_TRUE(count_black(*frame) > 0);
    // Nothing is drawn to the right of the right edge or above the top.
    for (int y = 0; y < FrameBuffer::kHeight; ++y) {
        for (int x = 201; x < FrameBuffer::kWidth; ++x) {
            TEST_ASSERT_TRUE(frame->pixel(x, y) == Color::white);
        }
    }
    for (int x = 0; x < FrameBuffer::kWidth; ++x) {
        TEST_ASSERT_TRUE(frame->pixel(x, 9) == Color::white);
    }
}

void test_space_draws_nothing() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    mono_gfx::draw_text(*frame, mono_gfx::kFontLarge, 0, 0, "   ");
    TEST_ASSERT_EQUAL_INT(0, count_black(*frame));
}

}  // namespace

void run_mono_gfx_tests() {
    RUN_TEST(test_new_frame_is_white);
    RUN_TEST(test_rotation_270_maps_top_left_to_last_row);
    RUN_TEST(test_rotation_90_maps_top_left_to_first_row);
    RUN_TEST(test_out_of_bounds_pixels_are_ignored);
    RUN_TEST(test_fill_rect_is_clipped);
    RUN_TEST(test_draw_rect_outline);
    RUN_TEST(test_text_width_adds_advances);
    RUN_TEST(test_draw_text_alignment);
    RUN_TEST(test_space_draws_nothing);
}
