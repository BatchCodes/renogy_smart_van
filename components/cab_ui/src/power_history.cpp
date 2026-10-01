// SPDX-License-Identifier: GPL-3.0-or-later
#include "cab_ui/power_history.hpp"

#include <algorithm>

namespace cab_ui {

PowerHistory::PowerHistory(std::size_t capacity, std::uint32_t interval_ms)
    : samples_(std::max<std::size_t>(capacity, 1), 0.0F), interval_ms_(interval_ms) {}

bool PowerHistory::add(std::uint64_t now_ms, float power_w) {
    if (last_sample_ms_ && now_ms < *last_sample_ms_ + interval_ms_) {
        return false;
    }
    samples_[next_] = power_w;
    next_ = (next_ + 1) % samples_.size();
    count_ = std::min(count_ + 1, samples_.size());
    last_sample_ms_ = now_ms;
    return true;
}

std::vector<float> PowerHistory::values() const {
    std::vector<float> result;
    result.reserve(count_);
    const std::size_t first = (next_ + samples_.size() - count_) % samples_.size();
    for (std::size_t offset = 0; offset < count_; ++offset) {
        result.push_back(samples_[(first + offset) % samples_.size()]);
    }
    return result;
}

float PowerHistory::max_value() const {
    const auto all = values();
    return all.empty() ? 0.0F : *std::max_element(all.begin(), all.end());
}

}  // namespace cab_ui
