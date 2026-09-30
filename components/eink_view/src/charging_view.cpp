// SPDX-License-Identifier: GPL-3.0-or-later
#include "eink_view/charging_view.hpp"

#include <cmath>
#include <cstdio>

#include "mono_gfx/text.hpp"

namespace eink_view {

namespace {

using charging_data::ChargingData;
using charging_data::LinkState;
using mono_gfx::Align;
using mono_gfx::Color;
using mono_gfx::FrameBuffer;
using mono_gfx::kFontLarge;
using mono_gfx::kFontMedium;
using mono_gfx::kFontSmall;

constexpr int kMargin = 4;
constexpr int kRight = FrameBuffer::kWidth - kMargin;
constexpr int kValueColumn = 54;
constexpr int kDividerY = 55;
constexpr int kAuxRowY = 58;
constexpr int kStarterRowY = 80;
constexpr int kFooterY = FrameBuffer::kHeight - 16;

std::string format(const char* pattern, double value) {
    char text[24];
    std::snprintf(text, sizeof(text), pattern, value);
    return text;
}

void draw_solar(FrameBuffer& frame, const ChargingData& data) {
    mono_gfx::draw_text(frame, kFontSmall, kMargin, 1, "SOLAR");
    mono_gfx::draw_text(frame, kFontSmall, kRight, 1, charging_data::to_string(data.charge_state),
                        Align::right);

    const std::string power = format("%.0f", data.pv_power_w);
    const int power_width = mono_gfx::draw_text(frame, kFontLarge, kMargin, 10, power);
    mono_gfx::draw_text(frame, kFontMedium, kMargin + power_width + 3,
                        10 + kFontLarge.baseline - kFontMedium.baseline, "W");

    mono_gfx::draw_text(frame, kFontMedium, kRight, 14, format("%.1f V", data.pv_voltage_v),
                        Align::right);
    mono_gfx::draw_text(frame, kFontMedium, kRight, 33, format("%.1f A", data.pv_current_a),
                        Align::right);
}

// One battery row: a small label, the voltage and the current.
void draw_battery_row(FrameBuffer& frame, int y, const char* label, float voltage_v,
                      const char* current_pattern, float current_a) {
    mono_gfx::draw_text(frame, kFontSmall, kMargin, y + kFontMedium.baseline - kFontSmall.baseline,
                        label);
    mono_gfx::draw_text(frame, kFontMedium, kValueColumn, y, format("%.2f V", voltage_v));
    mono_gfx::draw_text(frame, kFontMedium, kRight, y, format(current_pattern, current_a),
                        Align::right);
}

void draw_batteries(FrameBuffer& frame, const ChargingData& data) {
    frame.draw_hline(0, kDividerY, FrameBuffer::kWidth, Color::black);
    // The auxiliary (house) battery current is signed: negative while the load is larger.
    draw_battery_row(frame, kAuxRowY, "AUX", data.battery_voltage_v, "%+.1f A",
                     data.battery_current_a);
    // On a DC-DC charger the alternator input is the starter battery.
    draw_battery_row(frame, kStarterRowY, "START", data.alternator_voltage_v, "%.1f A",
                     data.alternator_current_a);
}

void draw_footer(FrameBuffer& frame, const ChargingData& data, LinkState state,
                 std::uint64_t age_ms) {
    const std::string energy = format("Today %.2f kWh", data.energy_today_wh / 1000.0);
    mono_gfx::draw_text(frame, kFontSmall, kMargin, kFooterY, energy);

    const std::string age = "Updated " + format_age(age_ms);
    if (state != LinkState::stale) {
        mono_gfx::draw_text(frame, kFontSmall, kRight, kFooterY, age, Align::right);
        return;
    }
    // Inverted box, so old values are hard to miss.
    const int width = mono_gfx::text_width(kFontSmall, age) + 6;
    frame.fill_rect(kRight - width, kFooterY - 1, width + 2, kFontSmall.height + 1, Color::black);
    mono_gfx::draw_text(frame, kFontSmall, kRight - 2, kFooterY, age, Align::right, Color::white);
}

void draw_offline(FrameBuffer& frame, const charging_data::LatestReading& reading,
                  std::uint64_t now_ms) {
    constexpr int kCentre = FrameBuffer::kWidth / 2;
    mono_gfx::draw_text(frame, kFontMedium, kCentre, 30, "Renogy offline", Align::centre);
    const auto age = reading.age_ms(now_ms);
    const std::string detail = age ? "Last data " + format_age(*age) : std::string("Waiting for data");
    mono_gfx::draw_text(frame, kFontSmall, kCentre, 62, detail, Align::centre);
}

}  // namespace

std::string format_age(std::uint64_t age_ms) {
    const std::uint64_t seconds = age_ms / 1000;
    char text[24];
    if (seconds < 60) {
        std::snprintf(text, sizeof(text), "%llu s ago", static_cast<unsigned long long>(seconds));
    } else if (seconds < 3600) {
        std::snprintf(text, sizeof(text), "%llu min ago", static_cast<unsigned long long>(seconds / 60));
    } else {
        std::snprintf(text, sizeof(text), "%llu h ago", static_cast<unsigned long long>(seconds / 3600));
    }
    return text;
}

void render(FrameBuffer& frame, const charging_data::LatestReading& reading, std::uint64_t now_ms) {
    frame.clear(Color::white);
    const LinkState state = reading.link_state(now_ms);
    if (state == LinkState::offline || !reading.data()) {
        draw_offline(frame, reading, now_ms);
        return;
    }
    const ChargingData& data = *reading.data();
    draw_solar(frame, data);
    draw_batteries(frame, data);
    draw_footer(frame, data, state, reading.age_ms(now_ms).value_or(0));
}

}  // namespace eink_view
