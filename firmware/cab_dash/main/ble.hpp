// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "esp_err.h"

namespace cab_dash {

// Connects NimBLE on the ESP32-P4 to the BLE controller on the ESP32-C6, through
// esp_hosted over SDIO, and logs the C6 firmware version. Pass it to
// renogy_ble::Bt2BleSource::start().
esp_err_t connect_c6_ble_controller();

}  // namespace cab_dash
