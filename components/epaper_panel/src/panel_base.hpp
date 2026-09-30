// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "epaper_panel/panel.hpp"
#include "panel_io.hpp"

namespace epaper_panel {

inline constexpr std::size_t kFrameBytes = 16 * 250;
inline constexpr std::uint32_t kFullRefreshTimeoutMs = 10'000;
inline constexpr std::uint32_t kFastRefreshTimeoutMs = 5'000;

// Keeps a copy of the last frame. A fast refresh sends it as the "old" image, so the
// controller does not need to keep its image memory between refreshes. The first
// refresh after start-up is always a full refresh.
class PanelBase : public Panel {
public:
    explicit PanelBase(PanelIo& io) : io_(io) {}

    esp_err_t show(std::span<const std::uint8_t> frame, RefreshMode mode) final;

protected:
    virtual esp_err_t show_full(std::span<const std::uint8_t> frame) = 0;
    virtual esp_err_t show_fast(std::span<const std::uint8_t> previous,
                                std::span<const std::uint8_t> frame) = 0;

    PanelIo& io_;

private:
    std::array<std::uint8_t, kFrameBytes> previous_{};
    bool has_previous_ = false;
};

class Ssd1680Panel final : public PanelBase {
public:
    using PanelBase::PanelBase;
    [[nodiscard]] const char* name() const override { return "SSD1680 (Heltec V1.0)"; }
    esp_err_t sleep() override;

protected:
    esp_err_t show_full(std::span<const std::uint8_t> frame) override;
    esp_err_t show_fast(std::span<const std::uint8_t> previous,
                        std::span<const std::uint8_t> frame) override;

private:
    esp_err_t initialise();
};

class Ssd1682Panel final : public PanelBase {
public:
    using PanelBase::PanelBase;
    [[nodiscard]] const char* name() const override { return "SSD1682 (Heltec V1.1.1/V1.2)"; }
    esp_err_t sleep() override;

protected:
    esp_err_t show_full(std::span<const std::uint8_t> frame) override;
    esp_err_t show_fast(std::span<const std::uint8_t> previous,
                        std::span<const std::uint8_t> frame) override;

private:
    esp_err_t initialise(RefreshMode mode);
};

class Jd79656Panel final : public PanelBase {
public:
    using PanelBase::PanelBase;
    [[nodiscard]] const char* name() const override { return "JD79656 (Heltec V1.1)"; }
    esp_err_t sleep() override;

protected:
    esp_err_t show_full(std::span<const std::uint8_t> frame) override;
    esp_err_t show_fast(std::span<const std::uint8_t> previous,
                        std::span<const std::uint8_t> frame) override;

private:
    esp_err_t reset();
    esp_err_t refresh_and_power_off(std::uint32_t timeout_ms);
};

}  // namespace epaper_panel
