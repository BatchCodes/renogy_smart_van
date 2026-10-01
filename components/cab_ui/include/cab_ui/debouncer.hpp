// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>

namespace cab_ui {

// Accepts a new input level only after it stays the same for the debounce time.
class Debouncer {
public:
    Debouncer(bool initial_level, std::uint32_t debounce_ms)
        : stable_(initial_level), candidate_(initial_level), debounce_ms_(debounce_ms) {}

    // Feeds one raw sample. Returns the stable level.
    bool update(bool level, std::uint64_t now_ms);

    [[nodiscard]] bool level() const { return stable_; }

private:
    bool stable_;
    bool candidate_;
    std::optional<std::uint64_t> candidate_since_ms_;
    std::uint32_t debounce_ms_;
};

}  // namespace cab_ui
