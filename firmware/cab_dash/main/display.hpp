// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "bsp/esp-bsp.h"
#include "esp_err.h"

namespace cab_dash {

// Holds the LVGL lock for one scope. LVGL calls from any task other than the LVGL
// task must run inside this lock.
class DisplayLock {
public:
    DisplayLock() : locked_(bsp_display_lock(0) == ESP_OK) {}
    ~DisplayLock() {
        if (locked_) {
            bsp_display_unlock();
        }
    }
    DisplayLock(const DisplayLock&) = delete;
    DisplayLock& operator=(const DisplayLock&) = delete;

    [[nodiscard]] bool locked() const { return locked_; }

private:
    bool locked_;
};

}  // namespace cab_dash
