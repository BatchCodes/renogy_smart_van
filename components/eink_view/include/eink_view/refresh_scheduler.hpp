// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace eink_view {

enum class Refresh : std::uint8_t { none, fast, full };

struct RefreshConfig {
    // A full refresh follows this many fast refreshes, to remove ghosting.
    std::uint32_t full_every_fast = 20;
    // A full refresh also happens when the image has not changed for this long.
    std::uint32_t max_unchanged_ms = 3'600'000;
};

// Decides whether a new frame needs a refresh, and which kind. The display refreshes
// only when the image changes, so an unchanged screen costs no power or panel wear.
class RefreshScheduler {
public:
    explicit RefreshScheduler(RefreshConfig config) : config_(config) {}

    [[nodiscard]] Refresh decide(std::span<const std::uint8_t> frame, std::uint64_t now_ms) const;

    // Call after a successful refresh.
    void shown(std::span<const std::uint8_t> frame, Refresh refresh, std::uint64_t now_ms);

private:
    [[nodiscard]] static std::uint32_t hash(std::span<const std::uint8_t> frame);

    RefreshConfig config_;
    std::optional<std::uint32_t> shown_hash_;
    std::uint64_t shown_at_ms_ = 0;
    std::uint32_t fast_since_full_ = 0;
};

}  // namespace eink_view
