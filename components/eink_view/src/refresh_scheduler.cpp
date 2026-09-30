// SPDX-License-Identifier: GPL-3.0-or-later
#include "eink_view/refresh_scheduler.hpp"

namespace eink_view {

std::uint32_t RefreshScheduler::hash(std::span<const std::uint8_t> frame) {
    // FNV-1a. A collision only skips one refresh, and the next change shows it.
    std::uint32_t value = 2166136261U;
    for (const std::uint8_t byte : frame) {
        value = (value ^ byte) * 16777619U;
    }
    return value;
}

Refresh RefreshScheduler::decide(std::span<const std::uint8_t> frame, std::uint64_t now_ms) const {
    if (!shown_hash_) {
        return Refresh::full;
    }
    const bool changed = hash(frame) != *shown_hash_;
    const std::uint64_t unchanged_ms = now_ms >= shown_at_ms_ ? now_ms - shown_at_ms_ : 0;
    if (!changed) {
        return unchanged_ms >= config_.max_unchanged_ms ? Refresh::full : Refresh::none;
    }
    return fast_since_full_ + 1 >= config_.full_every_fast ? Refresh::full : Refresh::fast;
}

void RefreshScheduler::shown(std::span<const std::uint8_t> frame, Refresh refresh,
                             std::uint64_t now_ms) {
    if (refresh == Refresh::none) {
        return;
    }
    shown_hash_ = hash(frame);
    shown_at_ms_ = now_ms;
    fast_since_full_ = refresh == Refresh::full ? 0 : fast_since_full_ + 1;
}

}  // namespace eink_view
