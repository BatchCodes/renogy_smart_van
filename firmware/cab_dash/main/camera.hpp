// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstdint>

#include "esp_err.h"

namespace cab_dash {

// One decoded camera frame in RGB565.
struct CameraFrame {
    const std::uint8_t* pixels = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t sequence = 0;  // Increases by one for each decoded frame.
};

// Reads MJPEG frames from a USB video class device (the CVBS capture adapter) and
// decodes them with the ESP32-P4 hardware JPEG decoder. Triple buffering: the decoder
// writes to a buffer that is neither the newest frame nor the frame on screen, so the
// screen never shows a half-written frame.
class Camera {
public:
    static Camera& instance();

    // Starts the USB host, the UVC driver and the camera task.
    esp_err_t start();

    // The newest decoded frame, or a frame with no pixels if there is none yet.
    [[nodiscard]] CameraFrame latest();

    // Call when the screen starts to show the frame with this sequence number. The
    // decoder then does not write to its buffer until the screen shows a newer frame.
    void displaying(std::uint32_t sequence);

    // True if a frame arrived in the last second.
    [[nodiscard]] bool has_signal(std::uint64_t now_ms) const;

    // Called by the camera task.
    void run();

private:
    Camera() = default;
};

}  // namespace cab_dash
