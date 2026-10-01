// SPDX-License-Identifier: GPL-3.0-or-later
#include "data_hub.hpp"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace cab_dash {

namespace {

constexpr const char* kTag = "data_hub";
constexpr std::uint32_t kTaskStackBytes = 4096;
constexpr UBaseType_t kTaskPriority = 4;

void poll_task(void* arg) {
    auto& hub = *static_cast<DataHub*>(arg);
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        hub.poll_once(now_ms());
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(hub.poll_interval_ms()));
    }
}

}  // namespace

std::uint64_t now_ms() { return static_cast<std::uint64_t>(esp_timer_get_time() / 1000); }

void DataHub::start(std::uint32_t poll_interval_ms) {
    poll_interval_ms_ = poll_interval_ms;
    if (xTaskCreate(poll_task, "data_hub", kTaskStackBytes, this, kTaskPriority, nullptr) != pdPASS) {
        ESP_LOGE(kTag, "Poll task not created");
    }
}

void DataHub::poll_once(std::uint64_t time_ms) {
    const auto data = source_.poll(time_ms);
    if (!data) {
        return;
    }
    const std::lock_guard lock(mutex_);
    reading_.update(*data, time_ms);
}

DataSnapshot DataHub::snapshot(std::uint64_t time_ms) {
    const std::lock_guard lock(mutex_);
    return {reading_.data(), reading_.link_state(time_ms), reading_.age_ms(time_ms)};
}

}  // namespace cab_dash
