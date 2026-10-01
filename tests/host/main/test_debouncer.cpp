// SPDX-License-Identifier: GPL-3.0-or-later
#include "cab_ui/debouncer.hpp"
#include "tests.hpp"
#include "unity.h"

using cab_ui::Debouncer;

namespace {

void test_change_needs_stable_time() {
    Debouncer input(false, 50);
    TEST_ASSERT_FALSE(input.update(true, 0));
    TEST_ASSERT_FALSE(input.update(true, 49));
    TEST_ASSERT_TRUE(input.update(true, 50));
    TEST_ASSERT_TRUE(input.level());
}

void test_glitch_is_ignored() {
    Debouncer input(false, 50);
    input.update(true, 0);
    input.update(false, 20);  // Back to the stable level: the glitch ends.
    TEST_ASSERT_FALSE(input.update(true, 30));
    TEST_ASSERT_FALSE(input.update(true, 79));
    TEST_ASSERT_TRUE(input.update(true, 80));
}

void test_release_is_debounced_too() {
    Debouncer input(true, 50);
    TEST_ASSERT_TRUE(input.update(false, 100));
    TEST_ASSERT_TRUE(input.update(false, 149));
    TEST_ASSERT_FALSE(input.update(false, 150));
}

void test_zero_debounce_follows_input() {
    Debouncer input(false, 0);
    TEST_ASSERT_TRUE(input.update(true, 5));
    TEST_ASSERT_FALSE(input.update(false, 6));
}

}  // namespace

void run_debouncer_tests() {
    RUN_TEST(test_change_needs_stable_time);
    RUN_TEST(test_glitch_is_ignored);
    RUN_TEST(test_release_is_debounced_too);
    RUN_TEST(test_zero_debounce_follows_input);
}
