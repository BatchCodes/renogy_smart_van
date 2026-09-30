// SPDX-License-Identifier: GPL-3.0-or-later
#include "panel_io.hpp"

#include <algorithm>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace epaper_panel {

namespace {

constexpr const char* kTag = "epaper_io";
constexpr int kSpiClockHz = 6'000'000;
constexpr std::size_t kMaxTransferBytes = 4096;
constexpr std::uint32_t kPowerSettleMs = 50;

}  // namespace

esp_err_t PanelIo::create(const PanelPins& pins, std::unique_ptr<PanelIo>& out) {
    std::unique_ptr<PanelIo> io(new PanelIo(pins));

    const gpio_config_t output_config = {
        .pin_bit_mask = (1ULL << pins.dc) | (1ULL << pins.rst),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&output_config), kTag, "DC and RST pins");
    gpio_set_level(pins.rst, 1);

    const gpio_config_t busy_config = {
        .pin_bit_mask = 1ULL << pins.busy,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&busy_config), kTag, "BUSY pin");

    if (pins.power != GPIO_NUM_NC) {
        const gpio_config_t power_config = {
            .pin_bit_mask = 1ULL << pins.power,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&power_config), kTag, "power pin");
        gpio_set_level(pins.power, pins.power_active_low ? 0 : 1);
        vTaskDelay(pdMS_TO_TICKS(kPowerSettleMs));
    }

    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = pins.mosi;
    bus_config.miso_io_num = GPIO_NUM_NC;
    bus_config.sclk_io_num = pins.sck;
    bus_config.quadwp_io_num = GPIO_NUM_NC;
    bus_config.quadhd_io_num = GPIO_NUM_NC;
    bus_config.max_transfer_sz = kMaxTransferBytes;
    ESP_RETURN_ON_ERROR(spi_bus_initialize(pins.spi_host, &bus_config, SPI_DMA_CH_AUTO), kTag,
                        "SPI bus");
    io->bus_initialised_ = true;

    spi_device_interface_config_t device_config = {};
    device_config.mode = 0;
    device_config.clock_speed_hz = kSpiClockHz;
    device_config.spics_io_num = pins.cs;
    device_config.queue_size = 1;
    ESP_RETURN_ON_ERROR(spi_bus_add_device(pins.spi_host, &device_config, &io->device_), kTag,
                        "SPI device");

    out = std::move(io);
    return ESP_OK;
}

PanelIo::~PanelIo() {
    if (device_ != nullptr) {
        spi_bus_remove_device(device_);
    }
    if (bus_initialised_) {
        spi_bus_free(pins_.spi_host);
    }
    if (pins_.power != GPIO_NUM_NC) {
        gpio_set_level(pins_.power, pins_.power_active_low ? 1 : 0);
    }
}

esp_err_t PanelIo::transmit(const std::uint8_t* bytes, std::size_t length, bool is_data) {
    gpio_set_level(pins_.dc, is_data ? 1 : 0);
    while (length > 0) {
        const std::size_t chunk = std::min(length, kMaxTransferBytes);
        spi_transaction_t transaction = {};
        transaction.length = chunk * 8;
        transaction.tx_buffer = bytes;
        ESP_RETURN_ON_ERROR(spi_device_polling_transmit(device_, &transaction), kTag, "SPI write");
        bytes += chunk;
        length -= chunk;
    }
    return ESP_OK;
}

esp_err_t PanelIo::command(std::uint8_t command) { return transmit(&command, 1, false); }

esp_err_t PanelIo::data(std::span<const std::uint8_t> bytes) {
    return transmit(bytes.data(), bytes.size(), true);
}

esp_err_t PanelIo::command(std::uint8_t command, std::initializer_list<std::uint8_t> bytes) {
    ESP_RETURN_ON_ERROR(this->command(command), kTag, "command 0x%02X", command);
    return data(std::span<const std::uint8_t>(bytes.begin(), bytes.size()));
}

void PanelIo::hardware_reset() {
    gpio_set_level(pins_.rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(pins_.rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

bool PanelIo::busy_level_during_reset() {
    gpio_set_level(pins_.rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    const bool level = gpio_get_level(pins_.busy) != 0;
    gpio_set_level(pins_.rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    return level;
}

esp_err_t PanelIo::wait_while_busy(bool busy_level, std::uint32_t timeout_ms) {
    const std::int64_t start_us = esp_timer_get_time();
    while ((gpio_get_level(pins_.busy) != 0) == busy_level) {
        if (esp_timer_get_time() - start_us > static_cast<std::int64_t>(timeout_ms) * 1000) {
            ESP_LOGE(kTag, "BUSY did not clear within %lu ms", static_cast<unsigned long>(timeout_ms));
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return ESP_OK;
}

}  // namespace epaper_panel
