#include "wake_radio.h"
#include "wake_protocol.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "br_c6.h"
#include "br_proto.h"
#include "btc_lmp.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "cis_c6.h"
#include "esp_bt.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "wake_radio";
typedef struct { uint8_t bytes[40]; uint16_t length; } radio_event_t;
static QueueHandle_t events;
static bool controller_started;
static bool adapter_started;
static volatile uint32_t traces;

/* Replace the upstream P4/SDIO transport with a bounded local HCI event queue.
 * Called by upstream's host TX task; never block the controller LL task. */
int slave_bt_send_to_host(const uint8_t *h4, uint16_t len)
{
    if (events && len <= sizeof(((radio_event_t *)0)->bytes)) {
        radio_event_t event = {.length = len};
        memcpy(event.bytes, h4, len);
        (void)xQueueSend(events, &event, 0);
    }
    return 0;
}

/* The standalone firmware does not include the LE Audio/CIS engine or P4 trace reader.
 * Preserve upstream crash protection; count its nonblocking diagnostic traces. */
void cis_c6_trace(void *context, const cis_trace_t *trace)
{
    (void)context; (void)trace;
    __atomic_fetch_add(&traces, 1u, __ATOMIC_RELAXED);
}

static void tx_ready(void) {}
static int controller_receive(uint8_t *data, uint16_t length)
{
    br_c6_c2h(data, length);
    return 0;
}
static esp_vhci_host_callback_t callbacks = {
    .notify_host_send_available = tx_ready,
    .notify_host_recv = controller_receive,
};

static esp_err_t command(const uint8_t *packet, uint16_t length, uint16_t opcode)
{
    xQueueReset(events);
    if (!br_c6_h2c(packet, length)) return ESP_ERR_NOT_SUPPORTED;
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(2000)) {
        radio_event_t event;
        if (xQueueReceive(events, &event, pdMS_TO_TICKS(50)) != pdTRUE) continue;
        const uint8_t *p = event.bytes;
        if (event.length >= 7 && p[0] == 4) {
            if (p[1] == 0x0e && (uint16_t)(p[4] | (p[5] << 8)) == opcode)
                return p[6] == 0 ? ESP_OK : ESP_FAIL;
            if (p[1] == 0x0f && (uint16_t)(p[5] | (p[6] << 8)) == opcode)
                return p[3] == 0 ? ESP_OK : ESP_FAIL;
        }
    }
    return ESP_ERR_TIMEOUT;
}

/* ---- LMP: a real controller runs LMP setup (features, LMP_host_connection_req, ...) right after the
 * baseband link is up; Soft-BR leaves LMP to the host, so the upstream btc_lmp engine runs here. ---- */
static btc_lmp_t lmp;
static bool lmp_active;
static uint16_t lmp_tx_count, lmp_rx_count;
static volatile int last_conn_status = -1;
static volatile uint16_t last_lmp_rx;
static volatile int64_t last_time_us;

void wake_radio_last(int *conn_status, uint16_t *lmp_rx, int64_t *time_us)
{
    if (conn_status) *conn_status = last_conn_status;
    if (lmp_rx) *lmp_rx = last_lmp_rx;
    if (time_us) *time_us = last_time_us;
}

static void log_lmp(const char *dir, const uint8_t *pdu, uint8_t len)
{
    unsigned op = pdu[0] >> 1;
    if (op == 127 && len >= 2) ESP_LOGI(TAG, "LMP_%s ext_op=%u len=%u", dir, pdu[1], len);
    else ESP_LOGI(TAG, "LMP_%s op=%u len=%u%s", dir, op, len, op == 51 ? " (host_connection_req)" : "");
}

static void lmp_send(void *ctx, const uint8_t *pdu, uint8_t len)
{
    (void)ctx;
    if (!pdu || len == 0 || len > BR_MAX_DM1_BYTES) return;
    uint8_t h4[4 + 8 + BR_MAX_DM1_BYTES] = {1, BR_HCI_OP_VENDOR_BRC6 & 0xff, BR_HCI_OP_VENDOR_BRC6 >> 8, (uint8_t)(8 + len),
        BR_VENDOR_MAGIC_0, BR_VENDOR_MAGIC_1, BR_VENDOR_MAGIC_2, BR_VENDOR_MAGIC_3, BR_VS_SEND_LMP,
        BR_ACL_HANDLE_BASE & 0xff, BR_ACL_HANDLE_BASE >> 8, len};
    memcpy(&h4[12], pdu, len);
    ++lmp_tx_count;
    log_lmp("TX", pdu, len);
    if (!br_c6_h2c(h4, (uint16_t)(12 + len))) ESP_LOGW(TAG, "LMP_TX rejected by engine");
}

static void lmp_random(void *ctx, uint8_t *out, size_t len) { (void)ctx; esp_fill_random(out, len); }
static void lmp_start_enc(void *ctx, const uint8_t r[16], const uint8_t k[16])
{ (void)ctx; (void)r; (void)k; ESP_LOGW(TAG, "LMP: encryption requested (no link key, not started)"); }
static void lmp_name(void *ctx, const char *name) { (void)ctx; ESP_LOGI(TAG, "LMP: remote name '%s'", name); }
static void lmp_key(void *ctx, const uint8_t a[6], const uint8_t k[16])
{ (void)ctx; (void)a; (void)k; ESP_LOGW(TAG, "LMP: new link key generated (not stored)"); }
static void lmp_done(void *ctx, bool ok, uint8_t err)
{ (void)ctx; ESP_LOGW(TAG, "LMP_SETUP_DONE ok=%d reason=0x%02x", ok, err); }
static void lmp_slots(void *ctx, uint8_t slots, uint8_t why, uint8_t err)
{ (void)ctx; ESP_LOGI(TAG, "LMP: peer max slots=%u why=%u err=0x%02x", slots, why, err); }

/* Drains HCI events during the attempt: reports Connection Complete (0x00 link up, 0x04 page timeout,
 * 0x3E link failed after the page), runs LMP on the link, and logs disconnects. */
static void run_link(uint32_t window_ms, const uint8_t controller[6])
{
    bool outcome = false, link_up = false;
    lmp_active = false;
    lmp_tx_count = lmp_rx_count = 0;
    TickType_t start = xTaskGetTickCount(), last_tick = start;
    last_conn_status = -2;
    last_time_us = esp_timer_get_time();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(window_ms)) {
        if (lmp_active && (xTaskGetTickCount() - last_tick) >= pdMS_TO_TICKS(500)) {
            last_tick = xTaskGetTickCount();
            btc_lmp_on_tick(&lmp);
        }
        radio_event_t event;
        if (xQueueReceive(events, &event, pdMS_TO_TICKS(20)) != pdTRUE) continue;
        const uint8_t *p = event.bytes;
        if (event.length < 3 || p[0] != 4) continue;
        if (p[1] == 0x0e || p[1] == 0x0f) continue; /* completions of our own LMP send commands */
        if (p[1] == 0xff && event.length >= 11 && p[3] == BR_VENDOR_MAGIC_0 && p[4] == BR_VENDOR_MAGIC_1 &&
            p[5] == BR_VENDOR_MAGIC_2 && p[6] == BR_VENDOR_MAGIC_3) {
            if (p[7] == BR_VSEVT_LMP_RX && event.length >= 11u + p[10]) {
                ++lmp_rx_count;
                log_lmp("RX", &p[11], p[10]);
                if (lmp_active) btc_lmp_on_pdu(&lmp, &p[11], p[10]);
            } else {
                ESP_LOGI(TAG, "VENDOR_EVT sub=0x%02x len=%u", p[7], event.length);
            }
            continue;
        }
        char hex[3 * 24 + 1] = {0};
        for (unsigned i = 0; i < event.length && i < 24; ++i)
            snprintf(hex + 3 * i, 4, "%02x ", p[i]);
        ESP_LOGI(TAG, "HCI_EVT code=0x%02x len=%u: %s", p[1], event.length, hex);
        if (p[1] == 0x03 && event.length >= 12) {
            outcome = true;
            uint8_t status = p[3];
            last_conn_status = status;
            ESP_LOGW(TAG, "CONN_COMPLETE status=0x%02x handle=0x%04x (%s)", status,
                     (unsigned)(p[4] | (p[5] << 8)),
                     status == 0x00 ? "baseband link up" :
                     status == 0x04 ? "page timeout: PS5 did not respond" :
                     status == 0x3e ? "PS5 answered the page, link not established" : "other HCI error");
            if (status == 0x00) {
                link_up = true;
                static const btc_lmp_ops_t ops = {
                    .send_lmp = lmp_send, .start_encryption = lmp_start_enc, .random_bytes = lmp_random,
                    .on_remote_name = lmp_name, .on_link_key = lmp_key, .on_setup_done = lmp_done,
                    .on_peer_max_slots = lmp_slots,
                };
                uint8_t local_le[6];
                for (unsigned i = 0; i < 6; ++i) local_le[i] = controller[5 - i];
                btc_lmp_init(&lmp, &ops, local_le, "Wireless Controller", NULL);
                btc_lmp_start(&lmp, &p[6], BTC_LMP_GOAL_CONNECT); /* peer address, little-endian as in the event */
                lmp_active = true;
            }
        } else if (p[1] == 0x05 && event.length >= 7) {
            ESP_LOGW(TAG, "DISCONNECTED reason=0x%02x", p[6]);
            lmp_active = false;
            link_up = false;
        }
    }
    if (link_up) {
        /* End the link cleanly. Without LMP_detach the PS5 keeps the "controller" connected until its supervision
         * timeout, and the real DualSense (same address) cannot connect in the meantime. */
        static const uint8_t detach[] = {(7 << 1) | 0, 0x13}; /* LMP_detach, TID 0 (we are master): user ended */
        lmp_send(NULL, detach, sizeof(detach));
        TickType_t sent = xTaskGetTickCount();
        while ((xTaskGetTickCount() - sent) < pdMS_TO_TICKS(400)) {
            radio_event_t event;
            if (xQueueReceive(events, &event, pdMS_TO_TICKS(20)) == pdTRUE && event.length >= 7 && event.bytes[0] == 4
                && event.bytes[1] == 0x05) break; /* disconnected */
        }
        ESP_LOGI(TAG, "LMP_DETACH sent: link released for the real controller");
    }
    if (!outcome) ESP_LOGW(TAG, "CONN_COMPLETE not received within %lu ms", (unsigned long)window_ms);
    ESP_LOGI(TAG, "LMP summary: tx=%u rx=%u", lmp_tx_count, lmp_rx_count);
    last_lmp_rx = lmp_rx_count;
    lmp_active = false;
}

/* Turns Bluetooth off completely after an attempt: Soft-BR engine, then the controller. Each attempt then starts
 * from the same fresh state as after a restart (long-running Bluetooth state stopped answering after many hours),
 * and between attempts Wi-Fi does not share the radio with an idle Bluetooth controller. */
void wake_radio_shutdown(void)
{
    if (adapter_started || br_c6_on()) br_c6_deinit();
    adapter_started = false;
    if (controller_started) {
        esp_err_t disabled = esp_bt_controller_disable();
        esp_err_t deinit = esp_bt_controller_deinit();
        if (disabled != ESP_OK || deinit != ESP_OK)
            ESP_LOGW(TAG, "Bluetooth off: disable=%s deinit=%s", esp_err_to_name(disabled), esp_err_to_name(deinit));
        else ESP_LOGI(TAG, "Bluetooth off until the next wake attempt");
        controller_started = false;
    }
}

esp_err_t wake_radio_attempt(const uint8_t ps5[6], const uint8_t controller[6], bool derived_identity)
{
    if (!events) events = xQueueCreate(16, sizeof(radio_event_t));
    if (!events) return ESP_ERR_NO_MEM;
    if (!controller_started) {
        esp_bt_controller_config_t config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        ESP_RETURN_ON_ERROR(esp_bt_controller_init(&config), TAG, "controller init");
        ESP_RETURN_ON_ERROR(esp_bt_controller_enable(ESP_BT_MODE_BLE), TAG, "controller enable");
        ESP_RETURN_ON_ERROR(esp_vhci_host_register_callback(&callbacks), TAG, "VHCI callbacks");
        controller_started = true;
    }
    if (!adapter_started || !br_c6_on()) {
        if (br_c6_init() != 0) return ESP_FAIL;
        adapter_started = true;
    }
    /* Upstream derives a different local identity by default. Override it EXACTLY,
     * with adjust=0, so the FHS contains the paired controller's real address. */
    uint8_t identity[16];
    wake_build_identity(identity, controller);
    if (derived_identity) {
        identity[15] = 1; /* let Soft-BR adjust LAP[13:6] and pick a C6-friendly sync word */
        ESP_LOGW(TAG, "DIAGNOSTIC: derived (non-paired) identity; the PS5 will not wake from this attempt");
    }
    ESP_RETURN_ON_ERROR(command(identity, sizeof(identity), 0xfc31), TAG, "identity rejected");
    uint8_t page[17];
    wake_build_page(page, ps5);
    esp_err_t result = command(page, sizeof(page), 0x0405);
    if (result == ESP_OK) {
        ESP_LOGW(TAG, "PAGE_ACCEPTED: radio attempt only; PS5 power state is NOT verified");
        run_link(10000, controller);
    }
    uint8_t stop[] = {1, 0x31, 0xfc, 5, 'B', 'R', 'C', '6', 9};
    esp_err_t stopped = command(stop, sizeof(stop), 0xfc31);
    ESP_LOGI(TAG, "Attempt ended: command=%s stop=%s traces=%lu", esp_err_to_name(result),
             esp_err_to_name(stopped), (unsigned long)__atomic_load_n(&traces, __ATOMIC_RELAXED));
    if (stopped != ESP_OK) {
        br_c6_deinit();
        adapter_started = false;
        return stopped;
    }
    return result;
}
