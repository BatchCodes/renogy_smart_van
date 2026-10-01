// SPDX-License-Identifier: GPL-3.0-or-later
//
// UVC streaming follows the ESP-IDF example examples/peripherals/usb/host/uvc and the
// esp-usb usb_host_uvc component. Decoding uses the ESP32-P4 JPEG engine.

#include "camera.hpp"

#include <array>
#include <mutex>

#include "bsp/esp-bsp.h"
#include "data_hub.hpp"
#include "driver/jpeg_decode.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "usb/uvc_host.h"

namespace cab_dash {

namespace {

constexpr const char* kTag = "camera";
constexpr std::uint32_t kWidth = CONFIG_CAB_DASH_CAMERA_WIDTH;
constexpr std::uint32_t kHeight = CONFIG_CAB_DASH_CAMERA_HEIGHT;
constexpr std::size_t kBufferCount = 3;
constexpr std::uint64_t kSignalTimeoutMs = 1000;
constexpr std::uint32_t kStatsIntervalMs = 5000;
constexpr int kDecodeTimeoutMs = 100;
constexpr TickType_t kOpenTimeout = pdMS_TO_TICKS(5000);

struct State {
    std::mutex mutex;
    std::array<std::uint8_t*, kBufferCount> buffers{};
    std::size_t buffer_bytes = 0;
    std::array<std::uint32_t, kBufferCount> widths{};
    std::array<std::uint32_t, kBufferCount> heights{};
    int newest = -1;     // Buffer with the newest frame.
    int on_screen = -1;  // Buffer that the screen shows.
    std::uint32_t sequence = 0;
    std::atomic<std::uint64_t> last_frame_ms{0};

    jpeg_decoder_handle_t decoder = nullptr;
    QueueHandle_t frames = nullptr;
    std::atomic<uvc_host_stream_hdl_t> stream{nullptr};

    std::uint32_t frames_decoded = 0;
    std::uint32_t frames_failed = 0;
    std::int64_t decode_us_total = 0;
};

State state;

int free_buffer() {
    for (int index = 0; index < static_cast<int>(kBufferCount); ++index) {
        if (index != state.newest && index != state.on_screen) {
            return index;
        }
    }
    return -1;
}

// Runs in the UVC driver task. Queue the frame and decode it in the camera task.
bool on_frame(const uvc_host_frame_t* frame, void* /*context*/) {
    if (xQueueSend(state.frames, &frame, 0) != pdTRUE) {
        return true;  // The camera task is busy. Drop this frame.
    }
    return false;  // The camera task returns it with uvc_host_frame_return().
}

void on_stream_event(const uvc_host_stream_event_data_t* event, void* /*context*/) {
    switch (event->type) {
        case UVC_HOST_DEVICE_DISCONNECTED:
            ESP_LOGW(kTag, "Camera disconnected");
            uvc_host_stream_close(event->device_disconnected.stream_hdl);
            state.stream = nullptr;
            break;
        case UVC_HOST_TRANSFER_ERROR:
            ESP_LOGW(kTag, "USB transfer error: %s", esp_err_to_name(event->transfer_error.error));
            break;
        case UVC_HOST_FRAME_BUFFER_OVERFLOW:
            ESP_LOGW(kTag, "Frame larger than the frame buffer, dropped");
            break;
        case UVC_HOST_FRAME_BUFFER_UNDERFLOW:
            ESP_LOGD(kTag, "No free frame buffer, frame dropped");
            break;
        default:
            break;
    }
}

void decode(const uvc_host_frame_t* frame) {
    int target = -1;
    {
        const std::lock_guard lock(state.mutex);
        target = free_buffer();
    }
    if (target < 0) {
        return;
    }

    jpeg_decode_picture_info_t info{};
    if (jpeg_decoder_get_info(frame->data, frame->data_len, &info) != ESP_OK || info.width > kWidth ||
        info.height > kHeight) {
        ++state.frames_failed;
        return;
    }

    jpeg_decode_cfg_t config{};
    config.output_format = JPEG_DECODE_OUT_FORMAT_RGB565;
    config.rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR;
    config.conv_std = JPEG_YUV_RGB_CONV_STD_BT601;

    std::uint32_t written = 0;
    const std::int64_t start_us = esp_timer_get_time();
    const esp_err_t result =
        jpeg_decoder_process(state.decoder, &config, frame->data, frame->data_len,
                             state.buffers[target], state.buffer_bytes, &written);
    state.decode_us_total += esp_timer_get_time() - start_us;
    if (result != ESP_OK) {
        ++state.frames_failed;
        return;
    }

    const std::lock_guard lock(state.mutex);
    state.widths[target] = info.width;
    state.heights[target] = info.height;
    state.newest = target;
    ++state.sequence;
    ++state.frames_decoded;
    state.last_frame_ms = now_ms();
}

void log_stats() {
    const std::uint32_t decoded = state.frames_decoded;
    const std::int64_t average_us = decoded > 0 ? state.decode_us_total / decoded : 0;
    ESP_LOGI(kTag, "%.1f fps, decode %lld us per frame, %lu failed",
             decoded * 1000.0 / kStatsIntervalMs, average_us,
             static_cast<unsigned long>(state.frames_failed));
    state.frames_decoded = 0;
    state.frames_failed = 0;
    state.decode_us_total = 0;
}

esp_err_t open_stream() {
    uvc_host_stream_config_t config = {};
    config.event_cb = on_stream_event;
    config.frame_cb = on_frame;
    config.vs_format.h_res = kWidth;
    config.vs_format.v_res = kHeight;
    config.vs_format.fps = 0;  // Device default.
    config.vs_format.format = UVC_VS_FORMAT_MJPEG;
    config.advanced.number_of_frame_buffers = 3;
    config.advanced.number_of_urbs = 3;
    config.advanced.frame_heap_caps = MALLOC_CAP_SPIRAM;
    uvc_host_stream_hdl_t stream = nullptr;
    ESP_RETURN_ON_ERROR(uvc_host_stream_open(&config, kOpenTimeout, &stream), kTag,
                        "no %lux%lu MJPEG stream", static_cast<unsigned long>(kWidth),
                        static_cast<unsigned long>(kHeight));
    uvc_host_desc_print(stream);
    state.stream = stream;
    return uvc_host_stream_start(stream);
}

void camera_task(void* /*arg*/) {
    Camera::instance().run();
}

}  // namespace

Camera& Camera::instance() {
    static Camera camera;
    return camera;
}

esp_err_t Camera::start() {
    jpeg_decode_engine_cfg_t engine{};
    engine.timeout_ms = kDecodeTimeoutMs;
    ESP_RETURN_ON_ERROR(jpeg_new_decoder_engine(&engine, &state.decoder), kTag, "JPEG decoder");

    jpeg_decode_memory_alloc_cfg_t memory{};
    memory.buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER;
    for (auto& buffer : state.buffers) {
        buffer = static_cast<std::uint8_t*>(jpeg_alloc_decoder_mem(kWidth * kHeight * 2, &memory, &state.buffer_bytes));
        ESP_RETURN_ON_FALSE(buffer != nullptr, ESP_ERR_NO_MEM, kTag, "frame buffer");
    }

    state.frames = xQueueCreate(2, sizeof(const uvc_host_frame_t*));
    ESP_RETURN_ON_FALSE(state.frames != nullptr, ESP_ERR_NO_MEM, kTag, "frame queue");

    // The BSP installs the USB host library and its event task.
    ESP_RETURN_ON_ERROR(bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV, false), kTag, "USB host");

    uvc_host_driver_config_t driver = {};
    driver.driver_task_stack_size = 6 * 1024;
    driver.driver_task_priority = 6;
    driver.xCoreID = tskNO_AFFINITY;
    driver.create_background_task = true;
    ESP_RETURN_ON_ERROR(uvc_host_install(&driver), kTag, "UVC driver");

    ESP_RETURN_ON_FALSE(xTaskCreate(camera_task, "camera", 6 * 1024, nullptr, 5, nullptr) == pdPASS,
                        ESP_ERR_NO_MEM, kTag, "camera task");
    ESP_LOGI(kTag, "Waiting for a %lux%lu MJPEG camera", static_cast<unsigned long>(kWidth),
             static_cast<unsigned long>(kHeight));
    return ESP_OK;
}

void Camera::run() {
    TickType_t last_stats = xTaskGetTickCount();
    for (;;) {
        if (state.stream == nullptr && open_stream() != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        const uvc_host_frame_t* frame = nullptr;
        if (xQueueReceive(state.frames, &frame, pdMS_TO_TICKS(100)) == pdTRUE) {
            decode(frame);
            if (uvc_host_stream_hdl_t stream = state.stream; stream != nullptr) {
                uvc_host_frame_return(stream, const_cast<uvc_host_frame_t*>(frame));
            }
        }

        if (xTaskGetTickCount() - last_stats >= pdMS_TO_TICKS(kStatsIntervalMs)) {
            log_stats();
            last_stats = xTaskGetTickCount();
        }
    }
}

CameraFrame Camera::latest() {
    const std::lock_guard lock(state.mutex);
    if (state.newest < 0) {
        return {};
    }
    return {state.buffers[state.newest], state.widths[state.newest], state.heights[state.newest], state.sequence};
}

void Camera::displaying(std::uint32_t sequence) {
    const std::lock_guard lock(state.mutex);
    if (sequence == state.sequence) {
        state.on_screen = state.newest;
    }
}

bool Camera::has_signal(std::uint64_t time_ms) const {
    const std::uint64_t last = state.last_frame_ms;
    return last != 0 && time_ms - last < kSignalTimeoutMs;
}

}  // namespace cab_dash
