// SPDX-License-Identifier: GPL-3.0-or-later
#include "reverse_input.hpp"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "sdkconfig.h"

namespace cab_dash {

namespace {

constexpr const char* kTag = "reverse_input";

}  // namespace

esp_err_t ReverseInput::init() {
#if CONFIG_CAB_DASH_REVERSE_GPIO >= 0
    constexpr auto kPin = static_cast<gpio_num_t>(CONFIG_CAB_DASH_REVERSE_GPIO);
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << kPin;
    config.mode = GPIO_MODE_INPUT;
#if CONFIG_CAB_DASH_REVERSE_ACTIVE_LOW
    config.pull_up_en = GPIO_PULLUP_ENABLE;
#else
    config.pull_down_en = GPIO_PULLDOWN_ENABLE;
#endif
    ESP_RETURN_ON_ERROR(gpio_config(&config), kTag, "GPIO %d", CONFIG_CAB_DASH_REVERSE_GPIO);
    debouncer_ = cab_ui::Debouncer(false, CONFIG_CAB_DASH_REVERSE_DEBOUNCE_MS);
    enabled_ = true;
    ESP_LOGI(kTag, "Reverse input on GPIO%d", CONFIG_CAB_DASH_REVERSE_GPIO);
#else
    ESP_LOGI(kTag, "No reverse input configured");
#endif
    return ESP_OK;
}

bool ReverseInput::sample(std::uint64_t now_ms) {
    if (!enabled_) {
        return false;
    }
#if CONFIG_CAB_DASH_REVERSE_GPIO >= 0
    const bool high = gpio_get_level(static_cast<gpio_num_t>(CONFIG_CAB_DASH_REVERSE_GPIO)) != 0;
#if CONFIG_CAB_DASH_REVERSE_ACTIVE_LOW
    const bool reverse = !high;
#else
    const bool reverse = high;
#endif
    const bool before = debouncer_.level();
    const bool after = debouncer_.update(reverse, now_ms);
    if (after != before) {
        ESP_LOGI(kTag, "Reverse %s", after ? "on" : "off");
    }
    return after;
#else
    return false;
#endif
}

}  // namespace cab_dash
