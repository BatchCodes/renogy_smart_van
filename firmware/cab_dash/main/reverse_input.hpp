// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "cab_ui/debouncer.hpp"
#include "esp_err.h"

namespace cab_dash {

// Reads the reverse-gear input (refer to docs/renogy_displays/reverse_input.md) and
// debounces it. Without a configured GPIO, it always reports "not in reverse".
class ReverseInput {
public:
    esp_err_t init();

    // Reads the pin. Returns true while the van is in reverse, after the debounce.
    bool sample(std::uint64_t now_ms);

private:
    cab_ui::Debouncer debouncer_{false, 0};
    bool enabled_ = false;
};

}  // namespace cab_dash
