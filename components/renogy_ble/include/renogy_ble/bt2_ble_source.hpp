// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <mutex>
#include <optional>

#include "charging_data/charging_data.hpp"
#include "esp_err.h"

namespace renogy_ble {

// Reads a Renogy DC-DC charger through a BT-2 module over BLE, with NimBLE as the
// host stack. All BLE work runs in the NimBLE host task. poll() returns the newest
// complete reading, once.
//
// Sequence: scan for the BT-2 name (and MAC address, if set), connect, find the
// write characteristic 0xFFD1 (service 0xFFD0) and the notify characteristic 0xFFF1
// (service 0xFFF0), subscribe, read the model once, then read the charging block and
// the state block at the poll interval. On a disconnect, scan again after a back-off.
// The UUIDs and the sequence come from cyrils/renogy-bt and neilsheps/Renogy-BT2-Reader.
class Bt2BleSource final : public charging_data::ChargingDataSource {
public:
    // Only one instance can exist, because NimBLE has one host.
    static Bt2BleSource& instance();

    // Runs after NVS starts and before NimBLE starts. A board whose BLE controller is
    // on a co-processor (for example the ESP32-C6 next to an ESP32-P4) connects it
    // here. Other boards pass nothing.
    using BeforeNimbleInit = esp_err_t (*)();

    // Starts NVS (needed for the PHY calibration data), NimBLE and the host task.
    esp_err_t start(BeforeNimbleInit before_nimble_init = nullptr);

    std::optional<charging_data::ChargingData> poll(std::uint64_t now_ms) override;

    // Called from the NimBLE host task only.
    void publish(const charging_data::ChargingData& data);

private:
    Bt2BleSource() = default;

    std::mutex mutex_;
    std::optional<charging_data::ChargingData> pending_;
    bool started_ = false;
};

}  // namespace renogy_ble
