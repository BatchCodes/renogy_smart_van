// SPDX-License-Identifier: GPL-3.0-or-later
#include "tests.hpp"
#include "unity.h"

extern "C" void setUp() {}
extern "C" void tearDown() {}

int main() {
    UNITY_BEGIN();
    run_latest_reading_tests();
    run_fake_source_tests();
    run_mono_gfx_tests();
    run_eink_view_tests();
    return UNITY_END();
}
