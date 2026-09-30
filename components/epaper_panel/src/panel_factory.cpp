// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>

#include "esp_check.h"
#include "esp_log.h"
#include "panel_base.hpp"

namespace epaper_panel {

namespace {

constexpr const char* kTag = "epaper_panel";

}  // namespace

esp_err_t PanelBase::show(std::span<const std::uint8_t> frame, RefreshMode mode) {
    if (frame.size() != kFrameBytes) {
        ESP_LOGE(kTag, "frame is %u bytes, expected %u", static_cast<unsigned>(frame.size()),
                 static_cast<unsigned>(kFrameBytes));
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t result = ESP_OK;
    if (mode == RefreshMode::fast && has_previous_) {
        result = show_fast(previous_, frame);
    } else {
        result = show_full(frame);
    }

    // After a failed refresh the panel content is unknown, so the next refresh is full.
    has_previous_ = result == ESP_OK;
    if (has_previous_) {
        std::copy(frame.begin(), frame.end(), previous_.begin());
    }
    return result;
}

PanelDevice::PanelDevice(std::unique_ptr<PanelIo> io, std::unique_ptr<Panel> panel, PanelModel model)
    : io_(std::move(io)), panel_(std::move(panel)), model_(model) {}

PanelDevice::~PanelDevice() {
    // The panel uses the PanelIo, so it must go first.
    panel_.reset();
    io_.reset();
}

esp_err_t PanelDevice::create(const PanelPins& pins, PanelModel model, bool detect,
                              std::unique_ptr<PanelDevice>& out) {
    std::unique_ptr<PanelIo> io;
    ESP_RETURN_ON_ERROR(PanelIo::create(pins, io), kTag, "panel IO");

    if (detect) {
        // Solomon Systech controllers pull BUSY high while in reset, Fitipower pulls it low.
        const bool busy_high = io->busy_level_during_reset();
        model = busy_high ? PanelModel::heltec_v1_2_ssd1682 : PanelModel::heltec_v1_1_jd79656;
        ESP_LOGI(kTag, "BUSY is %s during reset: %s", busy_high ? "high" : "low", to_string(model));
    }

    std::unique_ptr<Panel> panel;
    switch (model) {
        case PanelModel::heltec_v1_0_ssd1680:
            panel = std::make_unique<Ssd1680Panel>(*io);
            break;
        case PanelModel::heltec_v1_1_jd79656:
            panel = std::make_unique<Jd79656Panel>(*io);
            break;
        case PanelModel::heltec_v1_2_ssd1682:
            panel = std::make_unique<Ssd1682Panel>(*io);
            break;
    }
    ESP_LOGI(kTag, "Using %s", panel->name());

    out.reset(new PanelDevice(std::move(io), std::move(panel), model));
    return ESP_OK;
}

const char* to_string(PanelModel model) {
    switch (model) {
        case PanelModel::heltec_v1_0_ssd1680:
            return "Heltec V1.0 (SSD1680)";
        case PanelModel::heltec_v1_1_jd79656:
            return "Heltec V1.1 (JD79656)";
        case PanelModel::heltec_v1_2_ssd1682:
            return "Heltec V1.1.1/V1.2 (SSD1682)";
    }
    return "unknown";
}

}  // namespace epaper_panel
