// SPDX-License-Identifier: GPL-3.0-or-later
#include "cab_ui/mode_switch.hpp"
#include "tests.hpp"
#include "unity.h"

using cab_ui::ModeSwitch;
using cab_ui::Screen;

namespace {

constexpr cab_ui::ModeSwitchConfig kConfig{.camera_hold_ms = 2000};

void test_starts_on_dashboard() {
    const ModeSwitch mode(kConfig);
    TEST_ASSERT_TRUE(mode.screen(0) == Screen::dashboard);
}

void test_touch_switches_screens() {
    ModeSwitch mode(kConfig);
    TEST_ASSERT_TRUE(mode.request(Screen::camera));
    TEST_ASSERT_TRUE(mode.screen(10) == Screen::camera);
    TEST_ASSERT_TRUE(mode.request(Screen::dashboard));
    TEST_ASSERT_TRUE(mode.screen(20) == Screen::dashboard);
}

void test_reverse_overrides_touch() {
    ModeSwitch mode(kConfig);
    mode.set_reverse(true, 100);
    TEST_ASSERT_TRUE(mode.screen(100) == Screen::camera);
    TEST_ASSERT_FALSE(mode.request(Screen::dashboard));
    TEST_ASSERT_TRUE(mode.screen(200) == Screen::camera);
}

void test_camera_holds_after_reverse_then_returns() {
    ModeSwitch mode(kConfig);
    mode.set_reverse(true, 0);
    mode.set_reverse(false, 1000);
    TEST_ASSERT_TRUE(mode.screen(2999) == Screen::camera);
    TEST_ASSERT_TRUE(mode.screen(3000) == Screen::dashboard);
}

void test_returns_to_manual_camera_selection() {
    ModeSwitch mode(kConfig);
    mode.request(Screen::camera);
    mode.set_reverse(true, 0);
    mode.set_reverse(false, 10);
    TEST_ASSERT_TRUE(mode.screen(5000) == Screen::camera);
}

void test_touch_during_hold_ends_hold() {
    ModeSwitch mode(kConfig);
    mode.set_reverse(true, 0);
    mode.set_reverse(false, 100);
    TEST_ASSERT_TRUE(mode.request(Screen::dashboard));
    TEST_ASSERT_TRUE(mode.screen(200) == Screen::dashboard);
}

void test_reverse_again_during_hold() {
    ModeSwitch mode(kConfig);
    mode.set_reverse(true, 0);
    mode.set_reverse(false, 100);
    mode.set_reverse(true, 500);
    TEST_ASSERT_TRUE(mode.screen(10'000) == Screen::camera);
    TEST_ASSERT_TRUE(mode.reverse());
}

}  // namespace

void run_mode_switch_tests() {
    RUN_TEST(test_starts_on_dashboard);
    RUN_TEST(test_touch_switches_screens);
    RUN_TEST(test_reverse_overrides_touch);
    RUN_TEST(test_camera_holds_after_reverse_then_returns);
    RUN_TEST(test_returns_to_manual_camera_selection);
    RUN_TEST(test_touch_during_hold_ends_hold);
    RUN_TEST(test_reverse_again_during_hold);
}
