// SPDX-License-Identifier: GPL-3.0-or-later
#include "cab_ui/debouncer.hpp"

namespace cab_ui {

bool Debouncer::update(bool level, std::uint64_t now_ms) {
    if (level == stable_) {
        candidate_ = level;
        candidate_since_ms_.reset();
        return stable_;
    }
    if (level != candidate_ || !candidate_since_ms_) {
        candidate_ = level;
        candidate_since_ms_ = now_ms;
    }
    if (now_ms - *candidate_since_ms_ >= debounce_ms_) {
        stable_ = level;
        candidate_since_ms_.reset();
    }
    return stable_;
}

}  // namespace cab_ui
