// SPDX-License-Identifier: GPL-3.0-or-later
#include "renogy_ble/bt2_ble_source.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string_view>

#include "esp_check.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"
#include "renogy_protocol/dc_charger.hpp"
#include "renogy_protocol/modbus.hpp"
#include "sdkconfig.h"

namespace renogy_ble {

namespace {

constexpr const char* kTag = "renogy_ble";

constexpr std::uint16_t kWriteServiceUuid = 0xFFD0;
constexpr std::uint16_t kWriteCharacteristicUuid = 0xFFD1;
constexpr std::uint16_t kNotifyServiceUuid = 0xFFF0;
constexpr std::uint16_t kNotifyCharacteristicUuid = 0xFFF1;
constexpr std::uint16_t kClientConfigUuid = 0x2902;

constexpr std::int32_t kScanDurationMs = 30'000;
constexpr std::int32_t kConnectTimeoutMs = 10'000;
constexpr std::uint32_t kFirstRetryDelayMs = 2'000;
constexpr std::uint32_t kMaxRetryDelayMs = 60'000;
// The Python reference waits 0.5 s between two reads.
constexpr std::uint32_t kDelayBetweenReadsMs = 500;

using renogy_protocol::RegisterBlock;

enum class Request : std::uint8_t { none, model, charging, state };

// Connection state. Only the NimBLE host task touches it.
struct Link {
    std::uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
    std::uint16_t service_end = 0;
    std::uint16_t write_handle = 0;
    std::uint16_t notify_handle = 0;
    std::uint16_t client_config_handle = 0;
    bool ready = false;
    bool model_read = false;
    Request pending = Request::none;
    Request next = Request::none;  // The next read in this poll cycle.
    std::uint32_t failed_reads = 0;
    std::uint32_t retry_delay_ms = kFirstRetryDelayMs;
    renogy_protocol::ResponseAssembler assembler;
    charging_data::ChargingData data;

    void clear() {
        conn_handle = BLE_HS_CONN_HANDLE_NONE;
        service_end = 0;
        write_handle = 0;
        notify_handle = 0;
        client_config_handle = 0;
        ready = false;
        pending = Request::none;
        next = Request::none;
        failed_reads = 0;
        assembler.reset();
    }
};

Link link;
ble_npl_callout poll_timer;
ble_npl_callout response_timer;
ble_npl_callout retry_timer;
ble_npl_callout next_read_timer;

int on_gap_event(ble_gap_event* event, void* arg);
void start_scan();

ble_uuid16_t uuid16(std::uint16_t value) { return BLE_UUID16_INIT(value); }

void format_address(const std::uint8_t* value, char* text, std::size_t size) {
    // NimBLE stores the address least significant byte first.
    std::snprintf(text, size, "%02X:%02X:%02X:%02X:%02X:%02X", value[5], value[4], value[3], value[2],
                  value[1], value[0]);
}

void schedule(ble_npl_callout& timer, std::uint32_t delay_ms) {
    ble_npl_callout_reset(&timer, ble_npl_time_ms_to_ticks32(delay_ms));
}

void schedule_retry() {
    ESP_LOGI(kTag, "Scanning again in %lu s", static_cast<unsigned long>(link.retry_delay_ms / 1000));
    schedule(retry_timer, link.retry_delay_ms);
    link.retry_delay_ms = std::min(link.retry_delay_ms * 2, kMaxRetryDelayMs);
}

void disconnect(const char* reason) {
    ESP_LOGW(kTag, "Disconnecting: %s", reason);
    if (link.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(link.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
}

RegisterBlock block_for(Request request) {
    switch (request) {
        case Request::model:
            return renogy_protocol::kModelBlock;
        case Request::charging:
            return renogy_protocol::kChargingBlock;
        case Request::state:
        case Request::none:
            break;
    }
    return renogy_protocol::kStateBlock;
}

void send_request(Request request) {
    if (!link.ready || link.pending != Request::none) {
        return;
    }
    const RegisterBlock block = block_for(request);
    const auto frame = renogy_protocol::build_read_request(
        static_cast<std::uint8_t>(CONFIG_RENOGY_BLE_DEVICE_ID), block.first_register, block.register_count);
    link.assembler.reset();
    const int rc = ble_gattc_write_no_rsp_flat(link.conn_handle, link.write_handle, frame.data(),
                                               static_cast<std::uint16_t>(frame.size()));
    if (rc != 0) {
        ESP_LOGW(kTag, "Write of read request 0x%04X failed; rc=%d", block.first_register, rc);
        return;
    }
    link.pending = request;
    schedule(response_timer, CONFIG_RENOGY_BLE_RESPONSE_TIMEOUT_MS);
}

void read_failed(const char* reason) {
    ++link.failed_reads;
    ESP_LOGW(kTag, "Read failed (%s), %lu in a row", reason, static_cast<unsigned long>(link.failed_reads));
    link.pending = Request::none;
    link.assembler.reset();
    if (link.failed_reads >= CONFIG_RENOGY_BLE_MAX_FAILED_READS) {
        disconnect("too many failed reads");
    }
}

void log_model(std::span<const std::uint8_t> frame) {
    if (renogy_protocol::check_read_response(frame, renogy_protocol::kModelBlock.register_count) !=
        renogy_protocol::FrameError::none) {
        return;
    }
    const auto text = frame.subspan(3, renogy_protocol::kModelBlock.register_count * 2);
    std::string_view model(reinterpret_cast<const char*>(text.data()), text.size());
    while (!model.empty() && (model.back() == ' ' || model.back() == '\0')) {
        model.remove_suffix(1);
    }
    ESP_LOGI(kTag, "Charger model: %.*s", static_cast<int>(model.size()), model.data());
}

void handle_frame(std::span<const std::uint8_t> frame) {
    ble_npl_callout_stop(&response_timer);
    ESP_LOG_BUFFER_HEX(kTag, frame.data(), frame.size());

    const Request request = link.pending;
    link.pending = Request::none;
    renogy_protocol::FrameError error = renogy_protocol::FrameError::none;

    switch (request) {
        case Request::model:
            link.model_read = true;
            log_model(frame);
            link.next = Request::charging;
            schedule(next_read_timer, kDelayBetweenReadsMs);
            return;
        case Request::charging:
            error = renogy_protocol::decode_charging_block(frame, link.data);
            if (error == renogy_protocol::FrameError::none) {
                link.next = Request::state;
                schedule(next_read_timer, kDelayBetweenReadsMs);
                return;
            }
            break;
        case Request::state:
            error = renogy_protocol::decode_state_block(frame, link.data);
            if (error == renogy_protocol::FrameError::none) {
                link.failed_reads = 0;
                link.retry_delay_ms = kFirstRetryDelayMs;
                Bt2BleSource::instance().publish(link.data);
                return;
            }
            break;
        case Request::none:
            ESP_LOGW(kTag, "Response with no request");
            return;
    }
    read_failed(renogy_protocol::to_string(error));
}

void on_notification(const ble_gap_event& event) {
    if (event.notify_rx.attr_handle != link.notify_handle) {
        return;
    }
    std::array<std::uint8_t, 256> chunk{};
    const std::uint16_t length = std::min<std::uint16_t>(OS_MBUF_PKTLEN(event.notify_rx.om), chunk.size());
    if (os_mbuf_copydata(event.notify_rx.om, 0, length, chunk.data()) != 0) {
        return;
    }
    switch (link.assembler.add(std::span(chunk.data(), length))) {
        case renogy_protocol::ResponseAssembler::Status::complete:
            handle_frame(link.assembler.frame());
            link.assembler.reset();
            break;
        case renogy_protocol::ResponseAssembler::Status::overflow:
            read_failed("response too long");
            break;
        case renogy_protocol::ResponseAssembler::Status::incomplete:
            break;
    }
}

void on_poll_timer(ble_npl_event* /*event*/) {
    schedule(poll_timer, CONFIG_RENOGY_BLE_POLL_INTERVAL_S * 1000U);
    if (link.pending != Request::none) {
        return;
    }
    send_request(link.model_read ? Request::charging : Request::model);
}

// The next read of a poll cycle: model (first cycle only), charging block, state block.
void on_next_read_timer(ble_npl_event* /*event*/) {
    const Request next = link.next;
    link.next = Request::none;
    if (next != Request::none) {
        send_request(next);
    }
}

void on_response_timer(ble_npl_event* /*event*/) { read_failed("timeout"); }

void on_retry_timer(ble_npl_event* /*event*/) { start_scan(); }

int on_client_config_written(std::uint16_t /*conn_handle*/, const ble_gatt_error* error,
                             ble_gatt_attr* /*attr*/, void* /*arg*/) {
    if (error->status != 0) {
        disconnect("subscribe failed");
        return 0;
    }
    ESP_LOGI(kTag, "Subscribed to notifications, reading every %d s", CONFIG_RENOGY_BLE_POLL_INTERVAL_S);
    link.ready = true;
    schedule(poll_timer, kDelayBetweenReadsMs);
    return 0;
}

int on_descriptor(std::uint16_t conn_handle, const ble_gatt_error* error, std::uint16_t /*chr_val_handle*/,
                  const ble_gatt_dsc* descriptor, void* /*arg*/) {
    if (error->status == 0) {
        if (ble_uuid_u16(&descriptor->uuid.u) == kClientConfigUuid) {
            link.client_config_handle = descriptor->handle;
        }
        return 0;
    }
    if (error->status != BLE_HS_EDONE || link.client_config_handle == 0) {
        disconnect("notify descriptor not found");
        return 0;
    }
    constexpr std::uint8_t kEnableNotifications[2] = {0x01, 0x00};
    ble_gattc_write_flat(conn_handle, link.client_config_handle, kEnableNotifications,
                         sizeof(kEnableNotifications), on_client_config_written, nullptr);
    return 0;
}

int on_notify_characteristic(std::uint16_t conn_handle, const ble_gatt_error* error,
                             const ble_gatt_chr* characteristic, void* /*arg*/) {
    if (error->status == 0) {
        link.notify_handle = characteristic->val_handle;
        return 0;
    }
    if (error->status != BLE_HS_EDONE || link.notify_handle == 0) {
        disconnect("characteristic 0xFFF1 not found");
        return 0;
    }
    ble_gattc_disc_all_dscs(conn_handle, link.notify_handle, link.service_end, on_descriptor, nullptr);
    return 0;
}

int on_notify_service(std::uint16_t conn_handle, const ble_gatt_error* error, const ble_gatt_svc* service,
                      void* /*arg*/) {
    if (error->status == 0) {
        link.service_end = service->end_handle;
        const ble_uuid16_t uuid = uuid16(kNotifyCharacteristicUuid);
        ble_gattc_disc_chrs_by_uuid(conn_handle, service->start_handle, service->end_handle, &uuid.u,
                                    on_notify_characteristic, nullptr);
        return 0;
    }
    if (error->status != BLE_HS_EDONE || link.service_end == 0) {
        disconnect("service 0xFFF0 not found");
    }
    return 0;
}

int on_write_characteristic(std::uint16_t conn_handle, const ble_gatt_error* error,
                            const ble_gatt_chr* characteristic, void* /*arg*/) {
    if (error->status == 0) {
        link.write_handle = characteristic->val_handle;
        return 0;
    }
    if (error->status != BLE_HS_EDONE || link.write_handle == 0) {
        disconnect("characteristic 0xFFD1 not found");
        return 0;
    }
    link.service_end = 0;
    const ble_uuid16_t uuid = uuid16(kNotifyServiceUuid);
    ble_gattc_disc_svc_by_uuid(conn_handle, &uuid.u, on_notify_service, nullptr);
    return 0;
}

int on_write_service(std::uint16_t conn_handle, const ble_gatt_error* error, const ble_gatt_svc* service,
                     void* /*arg*/) {
    if (error->status == 0) {
        link.service_end = service->end_handle;
        const ble_uuid16_t uuid = uuid16(kWriteCharacteristicUuid);
        ble_gattc_disc_chrs_by_uuid(conn_handle, service->start_handle, service->end_handle, &uuid.u,
                                    on_write_characteristic, nullptr);
        return 0;
    }
    if (error->status != BLE_HS_EDONE || link.service_end == 0) {
        disconnect("service 0xFFD0 not found");
    }
    return 0;
}

bool matches_configured_address(const ble_addr_t& address) {
    constexpr std::string_view kWanted = CONFIG_RENOGY_BLE_MAC_ADDRESS;
    if (kWanted.empty()) {
        return true;
    }
    char text[18];
    format_address(address.val, text, sizeof(text));
    return strncasecmp(text, kWanted.data(), kWanted.size()) == 0 && kWanted.size() == 17;
}

void on_discovery(const ble_gap_disc_desc& desc) {
    ble_hs_adv_fields fields;
    if (ble_hs_adv_parse_fields(&fields, desc.data, desc.length_data) != 0 || fields.name == nullptr) {
        return;
    }
    const std::string_view name(reinterpret_cast<const char*>(fields.name), fields.name_len);
#if CONFIG_RENOGY_BLE_LOG_ALL_DEVICES
    ESP_LOGI(kTag, "Seen %.*s, RSSI %d dBm", static_cast<int>(name.size()), name.data(), desc.rssi);
#endif
    constexpr std::string_view kPrefix = CONFIG_RENOGY_BLE_NAME_PREFIX;
    if (!name.starts_with(kPrefix)) {
        return;
    }

    char address[18];
    format_address(desc.addr.val, address, sizeof(address));
    ESP_LOGI(kTag, "Found %.*s at %s, RSSI %d dBm", static_cast<int>(name.size()), name.data(), address,
             desc.rssi);
    if (!matches_configured_address(desc.addr)) {
        ESP_LOGI(kTag, "Not the configured MAC address, ignored");
        return;
    }

    ble_gap_disc_cancel();
    std::uint8_t own_address_type = 0;
    ble_hs_id_infer_auto(0, &own_address_type);
    const int rc = ble_gap_connect(own_address_type, &desc.addr, kConnectTimeoutMs, nullptr, on_gap_event,
                                   nullptr);
    if (rc != 0) {
        ESP_LOGW(kTag, "Connect failed to start; rc=%d", rc);
        schedule_retry();
    }
}

int on_gap_event(ble_gap_event* event, void* /*arg*/) {
    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            on_discovery(event->disc);
            return 0;

        case BLE_GAP_EVENT_DISC_COMPLETE:
            if (link.conn_handle == BLE_HS_CONN_HANDLE_NONE && event->disc_complete.reason == 0) {
                ESP_LOGI(kTag, "No BT-2 found");
                schedule_retry();
            }
            return 0;

        case BLE_GAP_EVENT_CONNECT: {
            if (event->connect.status != 0) {
                ESP_LOGW(kTag, "Connection failed; status=%d", event->connect.status);
                schedule_retry();
                return 0;
            }
            link.clear();
            link.conn_handle = event->connect.conn_handle;
            ESP_LOGI(kTag, "Connected, discovering services");
            const ble_uuid16_t uuid = uuid16(kWriteServiceUuid);
            ble_gattc_disc_svc_by_uuid(link.conn_handle, &uuid.u, on_write_service, nullptr);
            return 0;
        }

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGW(kTag, "Disconnected; reason=0x%03X", event->disconnect.reason);
            ble_npl_callout_stop(&poll_timer);
            ble_npl_callout_stop(&response_timer);
            ble_npl_callout_stop(&next_read_timer);
            link.clear();
            schedule_retry();
            return 0;

        case BLE_GAP_EVENT_NOTIFY_RX:
            on_notification(*event);
            return 0;

        default:
            return 0;
    }
}

void start_scan() {
    if (link.conn_handle != BLE_HS_CONN_HANDLE_NONE || ble_gap_disc_active()) {
        return;
    }
    std::uint8_t own_address_type = 0;
    if (ble_hs_id_infer_auto(0, &own_address_type) != 0) {
        ESP_LOGE(kTag, "No BLE address");
        return;
    }
    ble_gap_disc_params params = {};
    // Active scan: the BT-2 name can be in the scan response.
    params.passive = 0;
    params.filter_duplicates = 1;
    ESP_LOGI(kTag, "Scanning for \"%s\"", CONFIG_RENOGY_BLE_NAME_PREFIX);
    const int rc = ble_gap_disc(own_address_type, kScanDurationMs, &params, on_gap_event, nullptr);
    if (rc != 0) {
        ESP_LOGW(kTag, "Scan failed to start; rc=%d", rc);
        schedule_retry();
    }
}

void on_sync() {
    ble_hs_util_ensure_addr(0);
    start_scan();
}

void on_reset(int reason) { ESP_LOGW(kTag, "NimBLE host reset; reason=%d", reason); }

void host_task(void* /*param*/) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

}  // namespace

Bt2BleSource& Bt2BleSource::instance() {
    static Bt2BleSource source;
    return source;
}

esp_err_t Bt2BleSource::start(BeforeNimbleInit before_nimble_init) {
    if (started_) {
        return ESP_OK;
    }

    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), kTag, "NVS erase");
        result = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(result, kTag, "NVS");
    if (before_nimble_init != nullptr) {
        ESP_RETURN_ON_ERROR(before_nimble_init(), kTag, "BLE controller set-up");
    }
    ESP_RETURN_ON_ERROR(nimble_port_init(), kTag, "NimBLE");

    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;

    ble_npl_eventq* queue = nimble_port_get_dflt_eventq();
    ble_npl_callout_init(&poll_timer, queue, on_poll_timer, nullptr);
    ble_npl_callout_init(&response_timer, queue, on_response_timer, nullptr);
    ble_npl_callout_init(&retry_timer, queue, on_retry_timer, nullptr);
    ble_npl_callout_init(&next_read_timer, queue, on_next_read_timer, nullptr);

    nimble_port_freertos_init(host_task);
    started_ = true;
    return ESP_OK;
}

void Bt2BleSource::publish(const charging_data::ChargingData& data) {
    ESP_LOGI(kTag, "PV %.1f V %.2f A %.0f W | AUX %.1f V %.2f A %u %% | START %.1f V %.2f A | %s | %lu Wh today",
             data.pv_voltage_v, data.pv_current_a, data.pv_power_w, data.battery_voltage_v,
             data.battery_current_a, data.battery_soc_percent, data.alternator_voltage_v,
             data.alternator_current_a, charging_data::to_string(data.charge_state),
             static_cast<unsigned long>(data.energy_today_wh));
    const std::lock_guard lock(mutex_);
    pending_ = data;
}

std::optional<charging_data::ChargingData> Bt2BleSource::poll(std::uint64_t /*now_ms*/) {
    const std::lock_guard lock(mutex_);
    auto data = pending_;
    pending_.reset();
    return data;
}

}  // namespace renogy_ble
