// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace cab_ui {

// Keeps one solar power sample per interval, for the energy page chart. When it is
// full, the oldest sample drops out.
class PowerHistory {
public:
    PowerHistory(std::size_t capacity, std::uint32_t interval_ms);

    // Adds a sample if at least one interval has passed since the last one.
    // Returns true if the sample was added.
    bool add(std::uint64_t now_ms, float power_w);

    // Samples from the oldest to the newest.
    [[nodiscard]] std::vector<float> values() const;
    [[nodiscard]] std::size_t size() const { return count_; }
    [[nodiscard]] std::size_t capacity() const { return samples_.size(); }
    [[nodiscard]] float max_value() const;

private:
    std::vector<float> samples_;
    std::size_t next_ = 0;
    std::size_t count_ = 0;
    std::uint32_t interval_ms_;
    std::optional<std::uint64_t> last_sample_ms_;
};

}  // namespace cab_ui
