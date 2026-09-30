// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <cstdio>
#include <memory>
#include <string>
#include <sys/stat.h>

#include "charging_data/fake_source.hpp"
#include "eink_view/charging_view.hpp"
#include "eink_view/refresh_scheduler.hpp"
#include "tests.hpp"
#include "unity.h"

using charging_data::ChargingData;
using charging_data::FakeSource;
using charging_data::FakeSourceConfig;
using charging_data::FreshnessConfig;
using charging_data::LatestReading;
using eink_view::Refresh;
using eink_view::RefreshConfig;
using eink_view::RefreshScheduler;
using mono_gfx::Color;
using mono_gfx::FrameBuffer;
using mono_gfx::Rotation;

namespace {

constexpr FreshnessConfig kFreshness{.stale_after_ms = 30'000, .offline_after_ms = 120'000};

// Writes the frame as a PBM image, so a person can check the layout with no board.
void write_preview(const FrameBuffer& frame, const char* name) {
    mkdir(PREVIEW_DIR, 0755);
    const std::string path = std::string(PREVIEW_DIR) + "/" + name + ".pbm";
    FILE* file = std::fopen(path.c_str(), "w");
    TEST_ASSERT_NOT_NULL(file);
    std::fprintf(file, "P1\n%d %d\n", FrameBuffer::kWidth, FrameBuffer::kHeight);
    for (int y = 0; y < FrameBuffer::kHeight; ++y) {
        for (int x = 0; x < FrameBuffer::kWidth; ++x) {
            std::fputc(frame.pixel(x, y) == Color::black ? '1' : '0', file);
        }
        std::fputc('\n', file);
    }
    std::fclose(file);
}

int count_black(const FrameBuffer& frame, int top, int bottom) {
    int count = 0;
    for (int y = top; y < bottom; ++y) {
        for (int x = 0; x < FrameBuffer::kWidth; ++x) {
            count += frame.pixel(x, y) == Color::black ? 1 : 0;
        }
    }
    return count;
}

ChargingData sample_data() {
    ChargingData data;
    data.pv_power_w = 327.0F;
    data.pv_voltage_v = 18.4F;
    data.pv_current_a = 17.8F;
    data.battery_voltage_v = 14.32F;
    data.battery_current_a = 22.9F;
    data.energy_today_wh = 1840;
    data.charge_state = charging_data::ChargeState::mppt;
    return data;
}

void test_format_age() {
    TEST_ASSERT_EQUAL_STRING("0 s ago", eink_view::format_age(999).c_str());
    TEST_ASSERT_EQUAL_STRING("59 s ago", eink_view::format_age(59'999).c_str());
    TEST_ASSERT_EQUAL_STRING("1 min ago", eink_view::format_age(60'000).c_str());
    TEST_ASSERT_EQUAL_STRING("59 min ago", eink_view::format_age(3'599'000).c_str());
    TEST_ASSERT_EQUAL_STRING("2 h ago", eink_view::format_age(7'200'000).c_str());
}

void test_render_connected() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    LatestReading reading(kFreshness);
    reading.update(sample_data(), 1'000);
    eink_view::render(*frame, reading, 2'000);
    write_preview(*frame, "connected");
    // The divider line is solid across the width.
    for (int x = 0; x < FrameBuffer::kWidth; ++x) {
        TEST_ASSERT_TRUE(frame->pixel(x, 72) == Color::black);
    }
    TEST_ASSERT_TRUE(count_black(*frame, 16, 64) > 300);
}

void test_render_stale_shows_age_box() {
    const auto connected = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    const auto stale = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    LatestReading reading(kFreshness);
    reading.update(sample_data(), 0);
    eink_view::render(*connected, reading, 1'000);
    eink_view::render(*stale, reading, 45'000);
    write_preview(*stale, "stale");
    const int footer_top = FrameBuffer::kHeight - 17;
    TEST_ASSERT_TRUE(count_black(*stale, footer_top, FrameBuffer::kHeight) >
                     count_black(*connected, footer_top, FrameBuffer::kHeight) + 100);
}

void test_render_offline_has_no_divider() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    LatestReading reading(kFreshness);
    reading.update(sample_data(), 0);
    eink_view::render(*frame, reading, 300'000);
    write_preview(*frame, "offline");
    TEST_ASSERT_TRUE(frame->pixel(0, 72) == Color::white);
    TEST_ASSERT_TRUE(count_black(*frame, 25, 80) > 100);
}

void test_render_waiting_for_first_data() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    const LatestReading reading(kFreshness);
    eink_view::render(*frame, reading, 0);
    write_preview(*frame, "waiting");
    TEST_ASSERT_TRUE(count_black(*frame, 0, FrameBuffer::kHeight) > 100);
}

void test_render_fake_night() {
    const auto frame = std::make_unique<FrameBuffer>(Rotation::clockwise_270);
    const FakeSource source(FakeSourceConfig{.day_length_ms = 100'000});
    LatestReading reading(kFreshness);
    reading.update(source.reading_at(75'000), 75'000);
    eink_view::render(*frame, reading, 75'000);
    write_preview(*frame, "fake_night");
    TEST_ASSERT_TRUE(count_black(*frame, 0, FrameBuffer::kHeight) > 100);
}

void test_first_frame_is_full_refresh() {
    const RefreshScheduler scheduler(RefreshConfig{});
    const std::array<std::uint8_t, 4> frame{1, 2, 3, 4};
    TEST_ASSERT_TRUE(scheduler.decide(frame, 0) == Refresh::full);
}

void test_unchanged_frame_is_skipped() {
    RefreshScheduler scheduler(RefreshConfig{.full_every_fast = 20, .max_unchanged_ms = 60'000});
    const std::array<std::uint8_t, 4> frame{1, 2, 3, 4};
    scheduler.shown(frame, Refresh::full, 0);
    TEST_ASSERT_TRUE(scheduler.decide(frame, 59'999) == Refresh::none);
    TEST_ASSERT_TRUE(scheduler.decide(frame, 60'000) == Refresh::full);
}

void test_changes_use_fast_then_full() {
    RefreshScheduler scheduler(RefreshConfig{.full_every_fast = 3, .max_unchanged_ms = 60'000});
    std::array<std::uint8_t, 4> frame{0, 0, 0, 0};
    scheduler.shown(frame, Refresh::full, 0);
    for (std::uint8_t step = 1; step <= 2; ++step) {
        frame[0] = step;
        TEST_ASSERT_TRUE(scheduler.decide(frame, step) == Refresh::fast);
        scheduler.shown(frame, Refresh::fast, step);
    }
    frame[0] = 9;
    TEST_ASSERT_TRUE(scheduler.decide(frame, 10) == Refresh::full);
    scheduler.shown(frame, Refresh::full, 10);
    frame[0] = 10;
    TEST_ASSERT_TRUE(scheduler.decide(frame, 11) == Refresh::fast);
}

}  // namespace

void run_eink_view_tests() {
    RUN_TEST(test_format_age);
    RUN_TEST(test_render_connected);
    RUN_TEST(test_render_stale_shows_age_box);
    RUN_TEST(test_render_offline_has_no_divider);
    RUN_TEST(test_render_waiting_for_first_data);
    RUN_TEST(test_render_fake_night);
    RUN_TEST(test_first_frame_is_full_refresh);
    RUN_TEST(test_unchanged_frame_is_skipped);
    RUN_TEST(test_changes_use_fast_then_full);
}
