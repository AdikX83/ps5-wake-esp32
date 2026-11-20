#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_app_desc.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"
#include "led.h"
#include "portal.h"
#include "ha_mqtt.h"
#include "ps5_state.h"
#include "wake_radio.h"
#include "wol.h"

typedef struct {
    uint32_t version;
    char ssid[33];
    char password[65];
    uint8_t ps5[6], controller[6], wol[6];
} settings_t;
static settings_t settings;
static QueueHandle_t requests;
static volatile bool connected, busy;
static volatile bool scanning;
static volatile uint16_t wifi_last_reason;
static esp_netif_t *station_netif;
static int64_t last_request;
static uint32_t accepted_packets;
static portMUX_TYPE request_lock = portMUX_INITIALIZER_UNLOCKED;
static const char *TAG = "ps5_wol";
/* portal_always: the panel on the home network ("Access from home network"), on by default; panel_https: HTTPS there. */
static bool configured, safe_mode, led_idle_off, led_dark, wol_ps5_set, portal_always = true, panel_https = true;
static bool update_check = true; /* the panel asks GitHub for new releases (from the browser, not the ESP32) */
static uint8_t wol_ps5[6];
static volatile bool sta_paused;
static volatile int64_t disconnected_since;

/* Settings added after version 1 live in their own NVS keys, so the settings blob stays compatible. */
static void load_extras(void)
{
    nvs_handle_t handle;
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) != ESP_OK) return;
    size_t size = sizeof(wol_ps5);
    wol_ps5_set = nvs_get_blob(handle, "wol_ps5", wol_ps5, &size) == ESP_OK && size == sizeof(wol_ps5);
    uint8_t flag = 0;
    if (nvs_get_u8(handle, "led_off", &flag) == ESP_OK) led_idle_off = flag != 0;
    flag = 0;
    if (nvs_get_u8(handle, "led_dark", &flag) == ESP_OK) led_dark = flag != 0;
    flag = 0;
    if (nvs_get_u8(handle, "portal_on", &flag) == ESP_OK) portal_always = flag != 0;
    if (nvs_get_u8(handle, "https", &flag) == ESP_OK) panel_https = flag != 0;
    if (nvs_get_u8(handle, "upd_check", &flag) == ESP_OK) update_check = flag != 0;
    nvs_close(handle);
}

static bool load_settings(void)
{
    nvs_handle_t handle;
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) != ESP_OK) return false;
    size_t length = sizeof(settings);
    esp_err_t result = nvs_get_blob(handle, "settings", &settings, &length);
    nvs_close(handle);
    return result == ESP_OK && length == sizeof(settings) && settings.version == 1
        && memchr(settings.ssid, 0, sizeof(settings.ssid)) && settings.ssid[0]
        && memchr(settings.password, 0, sizeof(settings.password));
}

static void restore_country(void)
{
    nvs_handle_t handle;
    char code[3] = {0};
    size_t size = sizeof(code);
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) != ESP_OK) return;
    esp_err_t result = nvs_get_str(handle, "country", code, &size);
    nvs_close(handle);
    if (result == ESP_OK && size == 3) {
        result = esp_wifi_set_country_code(code, true);
        ESP_LOGI(TAG, "Restored Wi-Fi country %c%c: %s", code[0], code[1], esp_err_to_name(result));
    }
}

static void set_country(const char *code)
{
    if (strlen(code) != 2 || code[0] < 'A' || code[0] > 'Z' || code[1] < 'A' || code[1] > 'Z') {
        printf("COUNTRY_RESULT ESP_ERR_INVALID_ARG\n");
        return;
    }
    scanning = true;
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(250));
    wifi_country_t previous = {0};
    esp_wifi_get_country(&previous);
    esp_err_t result = esp_wifi_set_country_code(code, true);
    if (result == ESP_OK) {
        nvs_handle_t handle;
        result = nvs_open("ps5_wol", NVS_READWRITE, &handle);
        if (result == ESP_OK) {
            result = nvs_set_str(handle, "country", code);
            if (result == ESP_OK) result = nvs_commit(handle);
            nvs_close(handle);
        }
        if (result != ESP_OK) {
            char old_code[3] = {previous.cc[0], previous.cc[1], 0};
            esp_wifi_set_country_code(old_code, previous.policy == WIFI_COUNTRY_POLICY_AUTO);
        }
    }
    printf("COUNTRY_RESULT %s\n", esp_err_to_name(result));
    scanning = false;
    esp_wifi_connect();
}

static esp_err_t store_settings(const char *text)
{
    cJSON *root = cJSON_Parse(text);
    if (!root) return ESP_ERR_INVALID_ARG;
    settings_t next = {.version = 1};
    const cJSON *ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    const cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "password");
    const cJSON *ps5 = cJSON_GetObjectItemCaseSensitive(root, "ps5_bt");
    const cJSON *pad = cJSON_GetObjectItemCaseSensitive(root, "controller_bt");
    const cJSON *wol = cJSON_GetObjectItemCaseSensitive(root, "wol_mac");
    esp_err_t result = ESP_ERR_INVALID_ARG;
    if (!cJSON_IsString(ssid) || !ssid->valuestring[0] || strlen(ssid->valuestring) > 32
        || !cJSON_IsString(password) || strlen(password->valuestring) > 64
        || !cJSON_IsString(ps5) || !parse_mac(ps5->valuestring, next.ps5)
        || !cJSON_IsString(pad) || !parse_mac(pad->valuestring, next.controller)) goto done;
    if (wol && (!cJSON_IsString(wol) || !parse_mac(wol->valuestring, next.wol))) goto done;
    if (!memcmp(next.ps5, next.controller, 6)) goto done;
    memcpy(next.ssid, ssid->valuestring, strlen(ssid->valuestring) + 1);
    memcpy(next.password, password->valuestring, strlen(password->valuestring) + 1);
    nvs_handle_t handle;
    result = nvs_open("ps5_wol", NVS_READWRITE, &handle);
    if (result == ESP_OK) {
        result = nvs_set_blob(handle, "settings", &next, sizeof(next));
        if (result == ESP_OK) result = nvs_commit(handle);
        nvs_close(handle);
    }
done:
    memset(next.password, 0, sizeof(next.password));
    cJSON_Delete(root);
    return result;
}

/* Reconnect 2 s after a disconnect: with WPA3/PMF the router keeps the old association for a moment and
 * immediate retries failed for minutes (reason 209, SA query timeout). */
static esp_timer_handle_t reconnect_timer;
static void reconnect_cb(void *arg)
{
    (void)arg;
    if (!connected && !scanning && !sta_paused) esp_wifi_connect();
}

static void schedule_reconnect(void)
{
    if (!reconnect_timer) {
        const esp_timer_create_args_t args = {.callback = reconnect_cb, .name = "wifi_reconnect"};
        if (esp_timer_create(&args, &reconnect_timer) != ESP_OK) {
            esp_wifi_connect();
            return;
        }
    }
    esp_timer_stop(reconnect_timer);
    esp_timer_start_once(reconnect_timer, 2000000);
}

/* Self-healing: a configured device that stays offline for 2 minutes (outside the panel) restarts. */
static void wifi_watch_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        if (configured && !connected && !sta_paused && (!portal_active() || portal_always) && !busy
            && esp_timer_get_time() - disconnected_since > 120000000) {
            ESP_LOGW(TAG, "WIFI_WATCHDOG: no connection for 2 min (reason %u), restarting", (unsigned)wifi_last_reason);
            vTaskDelay(pdMS_TO_TICKS(200));
            esp_restart();
        }
    }
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        if (configured && !sta_paused) esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (connected) disconnected_since = esp_timer_get_time();
        connected = false;
        if (!scanning && !sta_paused) {
            wifi_event_sta_disconnected_t *event = data;
            wifi_last_reason = event->reason;
            ESP_LOGW(TAG, "Wi-Fi disconnected: reason=%u", (unsigned)wifi_last_reason);
            schedule_reconnect();
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        connected = true;
        ip_event_got_ip_t *event = data;
        ESP_LOGI(TAG, "Wi-Fi ready: " IPSTR "; WoL: UDP 9/7 and Ethernet 0x0842", IP2STR(&event->ip_info.ip));
    }
}

static void scan_target(bool all_networks)
{
    scanning = true;
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(250));
    wifi_scan_config_t scan = {.ssid = all_networks ? NULL : (uint8_t *)settings.ssid, .show_hidden = true};
    esp_err_t result = esp_wifi_scan_start(&scan, true);
    wifi_ap_record_t record = {0}, best = {0};
    uint16_t total = 0, matches = 0, channels[15] = {0};
    if (result == ESP_OK) {
        while (esp_wifi_scan_get_ap_record(&record) == ESP_OK) {
            ++total;
            if (record.primary < 15) ++channels[record.primary];
            if (!strcmp((const char *)record.ssid, settings.ssid)) {
                if (matches == 0 || record.rssi > best.rssi) best = record;
                ++matches;
            }
        }
        esp_wifi_clear_ap_list();
    }
    wifi_country_t country = {0};
    esp_wifi_get_country(&country);
    printf("SCAN_DONE result=%s total=%u matches=%u channel=%u rssi=%d auth=%u country=%c%c first_channel=%u channel_count=%u seen_channels=",
           esp_err_to_name(result), total, matches, best.primary, best.rssi, (unsigned)best.authmode,
           country.cc[0], country.cc[1], country.schan, country.nchan);
    for (unsigned channel = 1; channel < 15; ++channel)
        if (channels[channel]) printf("%u:%u,", channel, channels[channel]);
    printf("\n");
    scanning = false;
    esp_wifi_connect();
}

static bool request_wake(uint8_t token)
{
    int64_t now = esp_timer_get_time();
    portENTER_CRITICAL(&request_lock);
    if (busy || (last_request != 0 && now - last_request < 30000000)) {
        portEXIT_CRITICAL(&request_lock);
        return false;
    }
    last_request = now;
    busy = true;
    portEXIT_CRITICAL(&request_lock);
    if (xQueueSend(requests, &token, 0) != pdTRUE) {
        busy = false;
        return false;
    }
    return true;
}

/* Erases every setting of this project (namespace "ps5_wol") and restarts unconfigured. */
static void factory_reset(void)
{
    ESP_LOGW(TAG, "FACTORY_RESET: erasing all settings and restarting");
    nvs_handle_t handle;
    if (nvs_open("ps5_wol", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
    }
    vTaskDelay(pdMS_TO_TICKS(1500)); /* let the purple blink show */
    esp_restart();
}

/* BOOT button (GPIO9, active low) at runtime, decided by how long it was held:
 *   < 1 s, released: wake (the same as a WoL packet)     1-5 s, released: nothing (white LED)
 *   5-20 s, released: configuration panel (red LED, fast red blink from 15 s)
 *   20 s, still held: factory reset (purple fast blink), then restart
 * Held at power-up it still selects the safe mode, in which this task is not started. 50 ms debounce. */
static void button_task(void *arg)
{
    (void)arg;
    bool pressed = !gpio_get_level(GPIO_NUM_9), ignore = true;
    unsigned stable = 0;
    int64_t pressed_at = esp_timer_get_time();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
        int64_t now = esp_timer_get_time();
        bool raw = !gpio_get_level(GPIO_NUM_9);
        if (raw != pressed) {
            if (++stable >= 5) {
                stable = 0;
                pressed = raw;
                if (pressed) {
                    pressed_at = now;
                    ignore = false;
                } else {
                    int64_t held = now - pressed_at;
                    led_set_hold(0);
                    if (ignore) continue;
                    if (held < 1000000) {
                        led_event(LED_EVENT_TRIGGER);
                        if (app_request_wake()) ESP_LOGW(TAG, "BUTTON_WAKE: BOOT pressed, waking PS5");
                        else {
                            led_event(LED_EVENT_IGNORED);
                            ESP_LOGW(TAG, "BUTTON_WAKE ignored: not configured, attempt in progress or 30 s cooldown");
                        }
                    } else if (held >= 5000000) {
                        ESP_LOGW(TAG, "BUTTON_LONG: starting the configuration panel");
                        portal_start();
                    }
                }
            }
        } else stable = 0;
        if (pressed && !ignore) {
            int64_t held = now - pressed_at;
            if (held >= 20000000) {
                led_set_hold(4);
                factory_reset();
            } else if (held >= 15000000) led_set_hold(3);
            else if (held >= 5000000) led_set_hold(2);
            else if (held >= 1000000) led_set_hold(1);
        }
    }
}

static bool network_up(void) { return connected; }

static void power_changed(ps5_power_t state) { ha_mqtt_power_state(ps5_power_name(state)); }

static void wake_task(void *arg)
{
    (void)arg;
    uint8_t token;
    while (xQueueReceive(requests, &token, portMAX_DELAY) == pdTRUE) {
        busy = true;
        ESP_LOGW(TAG, "WAKE_ATTEMPT: experimental Soft-BR, console power state unverified");
        led_set_busy(true);
        ha_mqtt_wake_state("waking");
        ps5_state_kick();
        esp_err_t result = wake_radio_attempt(settings.ps5, settings.controller, token == 2);
        led_set_busy(false);
        int status = -1;
        uint16_t lmp_rx = 0;
        wake_radio_last(&status, &lmp_rx, NULL);
        led_event(status == 0 && lmp_rx > 0 ? LED_EVENT_SUCCESS : LED_EVENT_FAILURE);
        ha_mqtt_wake_state(status == 0 && lmp_rx > 0 ? "answered" : "no_answer");
        ps5_state_kick();
        ESP_LOGI(TAG, "WAKE_ATTEMPT_ENDED: %s; verify console physically", esp_err_to_name(result));
        busy = false;
    }
}

/* A received magic packet (UDP payload or raw Ethernet frame): wake if it targets this ESP32 or the PS5. */
static void handle_wol(const uint8_t *data, size_t length, const char *via)
{
    if (!configured || length > 1500) return;
    if (!wol_matches(data, length, settings.wol) && !(wol_ps5_set && wol_matches(data, length, wol_ps5))) return;
    ++accepted_packets;
    ESP_LOGI(TAG, "WOL_MATCH count=%lu via %s", (unsigned long)accepted_packets, via);
    led_event(LED_EVENT_TRIGGER);
    if (!request_wake(1)) led_event(LED_EVENT_IGNORED);
}

/* Routers (e.g. Asuswrt-Merlin "ether-wake -b") send WoL as a raw Ethernet frame, EtherType 0x0842, which the
 * IP stack drops. Every received Wi-Fi frame passes here first (linker --wrap) and goes on unchanged. */
static volatile uint32_t ether_wol_frames, odd_broadcasts;
esp_err_t __real_esp_netif_receive(esp_netif_t *esp_netif, void *buffer, size_t len, void *eb);
esp_err_t __wrap_esp_netif_receive(esp_netif_t *esp_netif, void *buffer, size_t len, void *eb)
{
    const uint8_t *frame = buffer;
    if (frame && len >= 14) {
        uint16_t type = (uint16_t)(frame[12] << 8 | frame[13]);
        if (type == 0x0842) {
            ++ether_wol_frames;
            const uint8_t *m = len >= 26 ? frame + 20 : frame;
            ESP_LOGI(TAG, "ETHER_WOL len=%u dst=%02x:%02x:%02x:%02x:%02x:%02x target=%02x:%02x:%02x:%02x:%02x:%02x",
                     (unsigned)len, frame[0], frame[1], frame[2], frame[3], frame[4], frame[5],
                     m[0], m[1], m[2], m[3], m[4], m[5]);
            if (len >= 14 + 102) handle_wol(frame + 14, len - 14, "ethernet");
        } else if (frame[0] == 0xff && type != 0x0800 && type != 0x0806 && type != 0x86dd && odd_broadcasts < 20) {
            ++odd_broadcasts; /* diagnostics: unusual broadcast EtherTypes (first 20 only) */
            ESP_LOGI(TAG, "RX_BCAST type=0x%04x len=%u", type, (unsigned)len);
        }
    }
    return __real_esp_netif_receive(esp_netif, buffer, len, eb);
}

/* UDP WoL on port 9 (default) and port 7 (used by some apps). */
static void udp_task(void *arg)
{
    const int port = (int)(intptr_t)arg;
    const char *via = port == 7 ? "udp/7" : "udp/9";
    uint8_t packet[1501];
    for (;;) {
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) { vTaskDelay(pdMS_TO_TICKS(1000)); continue; }
        struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = htonl(INADDR_ANY)};
        if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
            ESP_LOGE(TAG, "UDP bind %d failed: %d", port, errno);
            close(sock); vTaskDelay(pdMS_TO_TICKS(1000)); continue;
        }
        for (;;) {
            /* One sentinel byte ensures even truncated oversized UDP packets are rejected. */
            int length = recv(sock, packet, sizeof(packet), 0);
            if (length < 0) break;
            handle_wol(packet, (size_t)length, via);
        }
        close(sock);
    }
}

/* ---- Functions for the configuration panel (portal.c) and the status LED (led.c) ---- */
static void mac_text(char out[18], const uint8_t mac[6])
{
    snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

led_base_t app_led_base(void)
{
    if (safe_mode) return LED_BASE_SAFE;
    if (!configured) return LED_BASE_UNCONFIGURED; /* the setup panel is open by itself until the first save */
    if (portal_active()) return LED_BASE_PORTAL; /* access point on (the home-network panel does not change the LED) */
    if (connected) return led_idle_off ? LED_BASE_OFF : LED_BASE_READY;
    return esp_timer_get_time() - disconnected_since < 30000000 ? LED_BASE_CONNECTING : LED_BASE_NO_WIFI;
}

bool app_request_wake(void)
{
    return configured && request_wake(1);
}

bool app_portal_always(void)
{
    return portal_always && configured;
}

bool app_https(void)
{
    return panel_https;
}

bool app_configured(void)
{
    return configured;
}

void app_portal_pause_sta(void)
{
    if (portal_always) return; /* the panel stays on next to normal operation: keep reconnecting */
    sta_paused = true;
    if (!connected) esp_wifi_disconnect();
}

void app_get_panel_user(char out[33])
{
    nvs_handle_t handle;
    size_t size = 33;
    out[0] = 0;
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) == ESP_OK) {
        if (nvs_get_str(handle, "panel_user", out, &size) != ESP_OK) out[0] = 0;
        nvs_close(handle);
    }
    if (strlen(out) < 3) strcpy(out, PORTAL_DEFAULT_USER);
}

/* Optional device name from the panel ("Living room"): used in the address, Home Assistant and the page title. */
void app_get_device_name(char out[25])
{
    nvs_handle_t handle;
    size_t size = 25;
    out[0] = 0;
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) == ESP_OK) {
        if (nvs_get_str(handle, "dev_name", out, &size) != ESP_OK) out[0] = 0;
        nvs_close(handle);
    }
}

/* mDNS host name (without .local): the device name in lower-case letters/digits/dashes ("Salon PS5" -> salon-ps5),
 * or without a name "ps5-wake-" + the same 4 hex digits as the panel network PS5-WAKE-XXXX, unique in a network. */
void app_get_hostname(char out[40])
{
    /* Latin letters with accents (U+00C0..U+017F) become plain letters: "Pokój Kuby" -> "pokoj-kuby". */
    static const struct { uint16_t cp; char ascii[3]; } latin[] = {{0x0C0, "a"}, {0x0C1, "a"}, {0x0C2, "a"}, {0x0C3, "a"}, {0x0C4, "a"}, {0x0C5, "a"}, {0x0C6, "ae"}, {0x0C7, "c"}, {0x0C8, "e"}, {0x0C9, "e"}, {0x0CA, "e"}, {0x0CB, "e"}, {0x0CC, "i"}, {0x0CD, "i"}, {0x0CE, "i"}, {0x0CF, "i"}, {0x0D0, "d"}, {0x0D1, "n"}, {0x0D2, "o"}, {0x0D3, "o"}, {0x0D4, "o"}, {0x0D5, "o"}, {0x0D6, "o"}, {0x0D8, "o"}, {0x0D9, "u"}, {0x0DA, "u"}, {0x0DB, "u"}, {0x0DC, "u"}, {0x0DD, "y"}, {0x0DE, "th"}, {0x0DF, "ss"}, {0x0E0, "a"}, {0x0E1, "a"}, {0x0E2, "a"}, {0x0E3, "a"}, {0x0E4, "a"}, {0x0E5, "a"}, {0x0E6, "ae"}, {0x0E7, "c"}, {0x0E8, "e"}, {0x0E9, "e"}, {0x0EA, "e"}, {0x0EB, "e"}, {0x0EC, "i"}, {0x0ED, "i"}, {0x0EE, "i"}, {0x0EF, "i"}, {0x0F0, "d"}, {0x0F1, "n"}, {0x0F2, "o"}, {0x0F3, "o"}, {0x0F4, "o"}, {0x0F5, "o"}, {0x0F6, "o"}, {0x0F8, "o"}, {0x0F9, "u"}, {0x0FA, "u"}, {0x0FB, "u"}, {0x0FC, "u"}, {0x0FD, "y"}, {0x0FE, "th"}, {0x0FF, "y"}, {0x100, "a"}, {0x101, "a"}, {0x102, "a"}, {0x103, "a"}, {0x104, "a"}, {0x105, "a"}, {0x106, "c"}, {0x107, "c"}, {0x108, "c"}, {0x109, "c"}, {0x10A, "c"}, {0x10B, "c"}, {0x10C, "c"}, {0x10D, "c"}, {0x10E, "d"}, {0x10F, "d"}, {0x110, "d"}, {0x111, "d"}, {0x112, "e"}, {0x113, "e"}, {0x114, "e"}, {0x115, "e"}, {0x116, "e"}, {0x117, "e"}, {0x118, "e"}, {0x119, "e"}, {0x11A, "e"}, {0x11B, "e"}, {0x11C, "g"}, {0x11D, "g"}, {0x11E, "g"}, {0x11F, "g"}, {0x120, "g"}, {0x121, "g"}, {0x122, "g"}, {0x123, "g"}, {0x124, "h"}, {0x125, "h"}, {0x126, "h"}, {0x127, "h"}, {0x128, "i"}, {0x129, "i"}, {0x12A, "i"}, {0x12B, "i"}, {0x12C, "i"}, {0x12D, "i"}, {0x12E, "i"}, {0x12F, "i"}, {0x130, "i"}, {0x131, "i"}, {0x132, "ij"}, {0x133, "ij"}, {0x134, "j"}, {0x135, "j"}, {0x136, "k"}, {0x137, "k"}, {0x138, "k"}, {0x139, "l"}, {0x13A, "l"}, {0x13B, "l"}, {0x13C, "l"}, {0x13D, "l"}, {0x13E, "l"}, {0x13F, "l"}, {0x140, "l"}, {0x141, "l"}, {0x142, "l"}, {0x143, "n"}, {0x144, "n"}, {0x145, "n"}, {0x146, "n"}, {0x147, "n"}, {0x148, "n"}, {0x149, "n"}, {0x14A, "n"}, {0x14B, "n"}, {0x14C, "o"}, {0x14D, "o"}, {0x14E, "o"}, {0x14F, "o"}, {0x150, "o"}, {0x151, "o"}, {0x152, "oe"}, {0x153, "oe"}, {0x154, "r"}, {0x155, "r"}, {0x156, "r"}, {0x157, "r"}, {0x158, "r"}, {0x159, "r"}, {0x15A, "s"}, {0x15B, "s"}, {0x15C, "s"}, {0x15D, "s"}, {0x15E, "s"}, {0x15F, "s"}, {0x160, "s"}, {0x161, "s"}, {0x162, "t"}, {0x163, "t"}, {0x164, "t"}, {0x165, "t"}, {0x168, "u"}, {0x169, "u"}, {0x16A, "u"}, {0x16B, "u"}, {0x16C, "u"}, {0x16D, "u"}, {0x16E, "u"}, {0x16F, "u"}, {0x170, "u"}, {0x171, "u"}, {0x172, "u"}, {0x173, "u"}, {0x174, "w"}, {0x175, "w"}, {0x176, "y"}, {0x177, "y"}, {0x178, "y"}, {0x179, "z"}, {0x17A, "z"}, {0x17B, "z"}, {0x17C, "z"}, {0x17D, "z"}, {0x17E, "z"}, {0x17F, "s"}};
    char name[25], slug[25] = "";
    app_get_device_name(name);
    size_t n = 0;
    for (const unsigned char *c = (const unsigned char *)name; *c && n < 24; ++c) {
        const char *add = NULL;
        char one[2] = {0, 0};
        if ((c[0] & 0xE0) == 0xC0 && (c[1] & 0xC0) == 0x80) { /* two-byte UTF-8 */
            uint16_t cp = (uint16_t)(((c[0] & 0x1F) << 6) | (c[1] & 0x3F));
            ++c;
            for (size_t i = 0; i < sizeof(latin) / sizeof(latin[0]); ++i)
                if (latin[i].cp == cp) { add = latin[i].ascii; break; }
        } else if (*c < 0x80) {
            char ch = (char)*c;
            if (ch >= 'A' && ch <= 'Z') ch += 32;
            if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) { one[0] = ch; add = one; }
        } else {
            while ((c[1] & 0xC0) == 0x80) ++c; /* skip other multi-byte characters */
        }
        if (add) {
            for (; *add && n < 24; ++add) slug[n++] = *add;
        } else if (n && slug[n - 1] != '-') {
            slug[n++] = '-';
        }
    }
    while (n && slug[n - 1] == '-') --n;
    slug[n] = 0;
    if (slug[0]) {
        snprintf(out, 40, "%s", slug);
    } else {
        uint8_t mac[6] = {0};
        esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
        snprintf(out, 40, "ps5-wake-%02x%02x", mac[4], mac[5]);
    }
}

static bool device_name_valid(const char *text)
{
    if (strlen(text) > 24) return false;
    for (const unsigned char *c = (const unsigned char *)text; *c; ++c)
        if (*c < 0x20 || *c == '"' || *c == '\\' || *c == '<' || *c == '>') return false;
    return true;
}

static bool user_valid(const char *text)
{
    size_t length = strlen(text);
    if (length < 3 || length > 32) return false;
    for (const char *c = text; *c; ++c)
        if ((unsigned char)*c < 0x21 || *c == ':' || *c == 0x7f) return false;
    return true;
}

void app_get_ap_password(char out[65])
{
    nvs_handle_t handle;
    size_t size = 65;
    out[0] = 0;
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) == ESP_OK) {
        if (nvs_get_str(handle, "ap_pass", out, &size) != ESP_OK) out[0] = 0;
        nvs_close(handle);
    }
    if (strlen(out) < 8) strcpy(out, PORTAL_DEFAULT_AP_PASSWORD);
}

/* True when the owner has replaced the publicly documented default panel password. */
static bool ap_password_custom(void)
{
    char current[65];
    app_get_ap_password(current);
    bool custom = strcmp(current, PORTAL_DEFAULT_AP_PASSWORD) != 0;
    memset(current, 0, sizeof(current));
    return custom;
}

void app_status_json(cJSON *root)
{
    char text[20];
    cJSON_AddStringToObject(root, "version", esp_app_get_description()->version);
    cJSON_AddBoolToObject(root, "configured", configured);
    cJSON_AddStringToObject(root, "ssid", configured ? settings.ssid : "");
    cJSON_AddBoolToObject(root, "wifi_connected", connected);
    cJSON_AddNumberToObject(root, "wifi_reason", wifi_last_reason);
    esp_netif_ip_info_t ip = {0};
    if (station_netif) esp_netif_get_ip_info(station_netif, &ip);
    snprintf(text, sizeof(text), IPSTR, IP2STR(&ip.ip));
    cJSON_AddStringToObject(root, "ip", text);
    wifi_ap_record_t ap;
    if (connected && esp_wifi_sta_get_ap_info(&ap) == ESP_OK) cJSON_AddNumberToObject(root, "rssi", ap.rssi);
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    mac_text(text, mac);
    cJSON_AddStringToObject(root, "esp_mac", text);
    if (configured) {
        mac_text(text, settings.wol);
        cJSON_AddStringToObject(root, "wol_mac", text);
        mac_text(text, settings.ps5);
        cJSON_AddStringToObject(root, "ps5_bt", text);
        mac_text(text, settings.controller);
        cJSON_AddStringToObject(root, "controller_bt", text);
    }
    if (wol_ps5_set) {
        mac_text(text, wol_ps5);
        cJSON_AddStringToObject(root, "wol_ps5", text);
    }
    /* "country": the setting ("" = automatic, from the router); "country_active": what the Wi-Fi driver uses now
     * (with the automatic setting it follows the router after connecting). */
    char stored[3] = "";
    bool stored_set = false;
    nvs_handle_t settings_nvs;
    if (nvs_open("ps5_wol", NVS_READONLY, &settings_nvs) == ESP_OK) {
        size_t size = sizeof(stored);
        stored_set = nvs_get_str(settings_nvs, "country", stored, &size) == ESP_OK;
        if (!stored_set) stored[0] = 0;
        nvs_close(settings_nvs);
    }
    cJSON_AddStringToObject(root, "country", stored);
    cJSON_AddBoolToObject(root, "country_set", stored_set);
    wifi_country_t country = {0};
    if (esp_wifi_get_country(&country) == ESP_OK && country.cc[0] >= 'A') {
        char code[3] = {country.cc[0], country.cc[1], 0};
        cJSON_AddStringToObject(root, "country_active", code);
    }
    cJSON_AddNumberToObject(root, "wol_count", accepted_packets);
    cJSON_AddBoolToObject(root, "busy", busy);
    cJSON_AddBoolToObject(root, "led_idle_off", led_idle_off);
    cJSON_AddBoolToObject(root, "led_dark", led_dark);
    cJSON_AddBoolToObject(root, "update_check", update_check);
    cJSON_AddBoolToObject(root, "portal_always", portal_always);
    cJSON_AddBoolToObject(root, "https", panel_https);
    char panel_user[33];
    app_get_panel_user(panel_user);
    cJSON_AddStringToObject(root, "panel_user", panel_user);
    char device_name[25], hostname[40];
    app_get_device_name(device_name);
    app_get_hostname(hostname);
    cJSON_AddStringToObject(root, "device_name", device_name);
    cJSON_AddStringToObject(root, "hostname", hostname);
    ha_mqtt_status_json(root);
    char ps5_name[33], ps5_ip[16];
    ps5_power_t power = ps5_state_get(ps5_name, ps5_ip);
    cJSON_AddStringToObject(root, "ps5_power", ps5_power_name(power));
    if (ps5_name[0]) cJSON_AddStringToObject(root, "ps5_name", ps5_name);
    if (ps5_ip[0]) cJSON_AddStringToObject(root, "ps5_ip", ps5_ip);
    ps5_console_t found[4];
    int count = ps5_state_list(found, 4);
    cJSON *list = cJSON_AddArrayToObject(root, "consoles");
    for (int i = 0; i < count; ++i) {
        cJSON *item = cJSON_CreateObject();
        char mac[18];
        snprintf(mac, sizeof(mac), "%.2s:%.2s:%.2s:%.2s:%.2s:%.2s", found[i].mac, found[i].mac + 2, found[i].mac + 4,
                 found[i].mac + 6, found[i].mac + 8, found[i].mac + 10);
        for (char *c = mac; *c; ++c) if (*c >= 'a' && *c <= 'f') *c -= 32;
        cJSON_AddStringToObject(item, "mac", mac);
        cJSON_AddStringToObject(item, "name", found[i].name);
        cJSON_AddStringToObject(item, "ip", found[i].ip);
        cJSON_AddStringToObject(item, "state", ps5_power_name(found[i].state));
        cJSON_AddItemToArray(list, item);
    }
    cJSON_AddBoolToObject(root, "ap_custom", ap_password_custom());
    char lang[8];
    app_get_lang(lang);
    cJSON_AddStringToObject(root, "lang", lang);
    int status = -1;
    uint16_t lmp_rx = 0;
    int64_t when = 0;
    wake_radio_last(&status, &lmp_rx, &when);
    cJSON *last = cJSON_AddObjectToObject(root, "last_wake");
    cJSON_AddNumberToObject(last, "status", status);
    cJSON_AddNumberToObject(last, "lmp_rx", lmp_rx);
    cJSON_AddNumberToObject(last, "seconds_ago", status == -1 ? -1 : (double)((esp_timer_get_time() - when) / 1000000));
}

void app_scan_json(cJSON *networks)
{
    bool reconnect = false;
    if (!connected && !sta_paused && configured) {
        scanning = true;
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(200));
        reconnect = true;
    }
    wifi_scan_config_t scan = {.show_hidden = false};
    if (esp_wifi_scan_start(&scan, true) == ESP_OK) {
        uint16_t count = 24;
        wifi_ap_record_t *records = calloc(count, sizeof(*records));
        if (records && esp_wifi_scan_get_ap_records(&count, records) == ESP_OK) {
            for (unsigned i = 0; i < count; ++i) {
                const char *name = (const char *)records[i].ssid;
                if (!name[0]) continue;
                bool seen = false;
                for (unsigned j = 0; j < i; ++j) seen |= !strcmp(name, (const char *)records[j].ssid);
                if (seen) continue; /* the strongest access point of a network is listed first */
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "ssid", name);
                cJSON_AddNumberToObject(item, "rssi", records[i].rssi);
                cJSON_AddNumberToObject(item, "channel", records[i].primary);
                cJSON_AddBoolToObject(item, "open", records[i].authmode == WIFI_AUTH_OPEN);
                cJSON_AddItemToArray(networks, item);
            }
        } else esp_wifi_clear_ap_list();
        free(records);
    }
    if (reconnect) {
        scanning = false;
        esp_wifi_connect();
    }
}

/* Panel language (UI only), kept in NVS key "lang"; English when unset. */
void app_get_lang(char out[8])
{
    nvs_handle_t handle;
    size_t size = 8;
    out[0] = 0;
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) == ESP_OK) {
        if (nvs_get_str(handle, "lang", out, &size) != ESP_OK) out[0] = 0;
        nvs_close(handle);
    }
    if (!out[0]) strcpy(out, "en");
}

esp_err_t app_set_lang(const char *code)
{
    size_t length = code ? strlen(code) : 0;
    if (length < 2 || length > 5) return ESP_ERR_INVALID_ARG;
    for (size_t i = 0; i < length; ++i)
        if (!((code[i] >= 'a' && code[i] <= 'z') || code[i] == '-')) return ESP_ERR_INVALID_ARG;
    nvs_handle_t handle;
    esp_err_t result = nvs_open("ps5_wol", NVS_READWRITE, &handle);
    if (result != ESP_OK) return result;
    result = nvs_set_str(handle, "lang", code);
    if (result == ESP_OK) result = nvs_commit(handle);
    nvs_close(handle);
    return result;
}

/* Errors are short codes ("ssid", "bt", ...); the panel translates them. */
esp_err_t app_save_json(const cJSON *root, const char **error)
{
    const cJSON *ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    const cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "password");
    const cJSON *open = cJSON_GetObjectItemCaseSensitive(root, "open");
    const cJSON *ps5 = cJSON_GetObjectItemCaseSensitive(root, "ps5_bt");
    const cJSON *pad = cJSON_GetObjectItemCaseSensitive(root, "controller_bt");
    const cJSON *wol = cJSON_GetObjectItemCaseSensitive(root, "wol_ps5");
    const cJSON *country = cJSON_GetObjectItemCaseSensitive(root, "country");
    const cJSON *ap_password = cJSON_GetObjectItemCaseSensitive(root, "ap_password");
    const cJSON *led_off = cJSON_GetObjectItemCaseSensitive(root, "led_idle_off");
    const cJSON *led_dark_in = cJSON_GetObjectItemCaseSensitive(root, "led_dark");
    const cJSON *always = cJSON_GetObjectItemCaseSensitive(root, "portal_always");
    const cJSON *https = cJSON_GetObjectItemCaseSensitive(root, "https");
    const cJSON *upd_check_in = cJSON_GetObjectItemCaseSensitive(root, "update_check");
    const cJSON *panel_user = cJSON_GetObjectItemCaseSensitive(root, "panel_user");
    const cJSON *device_name = cJSON_GetObjectItemCaseSensitive(root, "device_name");
    settings_t next = settings;
    next.version = 1;
    uint8_t wol_next[6] = {0};
    bool wol_next_set = false;
    esp_err_t result = ESP_ERR_INVALID_ARG;
    if (!cJSON_IsString(ssid) || !ssid->valuestring[0] || strlen(ssid->valuestring) > 32) {
        *error = "ssid";
        goto done;
    }
    bool same_network = configured && !strcmp(ssid->valuestring, settings.ssid);
    if (cJSON_IsTrue(open)) next.password[0] = 0;
    else if (cJSON_IsString(password) && password->valuestring[0]) {
        size_t length = strlen(password->valuestring);
        if (length < 8 || length > 64) {
            *error = "wifipass";
            goto done;
        }
        memcpy(next.password, password->valuestring, length + 1);
    } else if (!same_network) {
        *error = "needpass";
        goto done;
    }
    memset(next.ssid, 0, sizeof(next.ssid));
    memcpy(next.ssid, ssid->valuestring, strlen(ssid->valuestring));
    if (!cJSON_IsString(ps5) || !parse_mac(ps5->valuestring, next.ps5)
        || !cJSON_IsString(pad) || !parse_mac(pad->valuestring, next.controller)) {
        *error = "bt";
        goto done;
    }
    if (!memcmp(next.ps5, next.controller, 6)) {
        *error = "btsame";
        goto done;
    }
    if (cJSON_IsString(wol) && wol->valuestring[0]) {
        if (!parse_mac(wol->valuestring, wol_next)) {
            *error = "wol";
            goto done;
        }
        wol_next_set = true;
    }
    const char *code = cJSON_IsString(country) ? country->valuestring : "";
    if (code[0] && (strlen(code) != 2 || code[0] < 'A' || code[0] > 'Z' || code[1] < 'A' || code[1] > 'Z')) {
        *error = "country";
        goto done;
    }
    const char *new_ap = cJSON_IsString(ap_password) ? ap_password->valuestring : "";
    if (new_ap[0] && (strlen(new_ap) < 8 || strlen(new_ap) > 63)) {
        *error = "ap";
        goto done;
    }
    /* The default panel password is published in the documentation: an own one is required to save. */
    if ((!new_ap[0] && !ap_password_custom()) || !strcmp(new_ap, PORTAL_DEFAULT_AP_PASSWORD)) {
        *error = "appass";
        goto done;
    }
    if (cJSON_IsString(panel_user) && panel_user->valuestring[0] && !user_valid(panel_user->valuestring)) {
        *error = "puser";
        result = ESP_ERR_INVALID_ARG;
        goto done;
    }
    if (cJSON_IsString(device_name) && !device_name_valid(device_name->valuestring)) {
        *error = "devname";
        result = ESP_ERR_INVALID_ARG;
        goto done;
    }
    result = ha_mqtt_check(root, error);
    if (result != ESP_OK) goto done;
    nvs_handle_t handle;
    result = nvs_open("ps5_wol", NVS_READWRITE, &handle);
    if (result != ESP_OK) {
        *error = "nvs";
        goto done;
    }
    result = nvs_set_blob(handle, "settings", &next, sizeof(next));
    if (result == ESP_OK) {
        if (wol_next_set) result = nvs_set_blob(handle, "wol_ps5", wol_next, sizeof(wol_next));
        else if (nvs_erase_key(handle, "wol_ps5") == ESP_ERR_NVS_NOT_FOUND) result = ESP_OK;
    }
    /* "" stores the automatic setting explicitly (the panel then stops suggesting a country) */
    if (result == ESP_OK && cJSON_IsString(country)) result = nvs_set_str(handle, "country", code);
    if (result == ESP_OK && new_ap[0]) result = nvs_set_str(handle, "ap_pass", new_ap);
    if (result == ESP_OK) result = nvs_set_u8(handle, "led_off", cJSON_IsTrue(led_off) ? 1 : 0);
    if (result == ESP_OK) result = nvs_set_u8(handle, "led_dark", cJSON_IsTrue(led_dark_in) ? 1 : 0);
    if (result == ESP_OK) result = nvs_set_u8(handle, "portal_on", cJSON_IsTrue(always) ? 1 : 0);
    if (result == ESP_OK) result = nvs_set_u8(handle, "https", cJSON_IsFalse(https) ? 0 : 1);
    if (result == ESP_OK) result = nvs_set_u8(handle, "upd_check", cJSON_IsFalse(upd_check_in) ? 0 : 1);
    if (result == ESP_OK && cJSON_IsString(panel_user) && panel_user->valuestring[0])
        result = nvs_set_str(handle, "panel_user", panel_user->valuestring);
    if (result == ESP_OK && cJSON_IsString(device_name)) result = nvs_set_str(handle, "dev_name", device_name->valuestring);
    if (result == ESP_OK) result = ha_mqtt_save(handle, root);
    if (result == ESP_OK) result = nvs_commit(handle);
    nvs_close(handle);
    if (result != ESP_OK) *error = "nvs";
    else ESP_LOGW(TAG, "PORTAL_SAVED ssid=%s", next.ssid);
done:
    memset(next.password, 0, sizeof(next.password));
    return result;
}

static void console(void)
{
    char line[768];
    for (;;) {
        if (!fgets(line, sizeof(line), stdin)) { clearerr(stdin); vTaskDelay(pdMS_TO_TICKS(20)); continue; }
        line[strcspn(line, "\r\n")] = 0;
        if (!strcmp(line, "STATUS")) {
            esp_netif_ip_info_t ip = {0};
            wifi_ap_record_t ap = {0};
            bool associated = false;
            if (station_netif) {
                esp_netif_get_ip_info(station_netif, &ip);
                associated = esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
            }
            printf("STATUS configured=%d wifi=%d busy=%d wol_mac=" MACSTR
                   " wol_count=%lu wifi_reason=%u associated=%d rssi=%d ip=" IPSTR,
                configured, connected, busy, MAC2STR(settings.wol), (unsigned long)accepted_packets,
                (unsigned)wifi_last_reason, associated, associated ? ap.rssi : 0, IP2STR(&ip.ip));
            if (wol_ps5_set) printf(" wol_ps5=" MACSTR, MAC2STR(wol_ps5));
            printf(" uptime_s=%lld reset_reason=%d ether_wol_frames=%lu ps5=%s version=%s\n", esp_timer_get_time() / 1000000,
                   (int)esp_reset_reason(), (unsigned long)ether_wol_frames, ps5_power_name(ps5_state_get(NULL, NULL)),
                   esp_app_get_description()->version);
        } else if (!strcmp(line, "SCAN") && configured && !busy) {
            scan_target(false);
        } else if (!strcmp(line, "SCAN_ALL") && configured && !busy) {
            scan_target(true);
        } else if (!strncmp(line, "COUNTRY ", 8) && configured && !busy) {
            set_country(line + 8);
        } else if (!strcmp(line, "RESTART") && !busy) {
            printf("RESTARTING\n");
            fflush(stdout);
            vTaskDelay(pdMS_TO_TICKS(250));
            esp_restart();
        } else if (!strcmp(line, "WAKE") && configured) {
            request_wake(1);
        } else if (!strcmp(line, "WAKE_DERIVED") && configured) {
            request_wake(2); /* diagnostic: radio link test with a C6-friendly, non-paired address */
        } else if (!strncmp(line, "CONFIG ", 7) && !busy) {
            esp_err_t result = store_settings(line + 7);
            memset(line, 0, sizeof(line));
            printf("CONFIG_RESULT %s\n", esp_err_to_name(result));
            fflush(stdout);
            if (result == ESP_OK) { vTaskDelay(pdMS_TO_TICKS(500)); esp_restart(); }
        } else printf("COMMANDS STATUS | CONFIG <json> | WAKE | WAKE_DERIVED | SCAN | SCAN_ALL | COUNTRY <ISO2> | RESTART\n");
        memset(line, 0, sizeof(line));
    }
}

void app_main(void)
{
    usb_serial_jtag_driver_config_t usb = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb.rx_buffer_size = 1024;
    usb.tx_buffer_size = 1024;
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb));
    usb_serial_jtag_vfs_use_driver();
    setvbuf(stdin, NULL, _IONBF, 0);
    ESP_ERROR_CHECK(nvs_flash_init()); /* Never erase unrelated NVS automatically. */
    configured = load_settings();
    load_extras();
    ha_mqtt_load();
    gpio_set_direction(GPIO_NUM_9, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_9, GPIO_PULLUP_ONLY);
    if (!gpio_get_level(GPIO_NUM_9)) {
        configured = false;
        safe_mode = true;
        ESP_LOGW(TAG, "BOOT held: safe configuration mode; Wi-Fi and radio disabled");
    }
    led_init();
    led_set_dark(led_dark && configured && !safe_mode); /* a new or safe-mode device always shows its state */
    requests = xQueueCreate(1, sizeof(uint8_t));
    ESP_ERROR_CHECK(requests ? ESP_OK : ESP_ERR_NO_MEM);
    disconnected_since = esp_timer_get_time();
    if (!safe_mode) {
        if (configured) {
            uint8_t zero[6] = {0};
            if (!memcmp(settings.wol, zero, 6)) ESP_ERROR_CHECK(esp_read_mac(settings.wol, ESP_MAC_WIFI_STA));
        }
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        station_netif = esp_netif_create_default_wifi_sta();
        if (!station_netif) abort();
        wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&init));
        restore_country(); /* No override is applied until a country is explicitly configured. */
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, NULL));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        if (configured) {
            wifi_config_t wifi = {0};
            memcpy(wifi.sta.ssid, settings.ssid, strlen(settings.ssid));
            memcpy(wifi.sta.password, settings.password, strlen(settings.password));
            wifi.sta.threshold.authmode = settings.password[0] ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
            wifi.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
            wifi.sta.pmf_cfg.capable = true;
            wifi.sta.pmf_cfg.required = false;
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
            memset(&wifi, 0, sizeof(wifi));
        }
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
        if (configured && (xTaskCreate(wake_task, "wake", 6144, NULL, 5, NULL) != pdPASS
                           || xTaskCreate(udp_task, "wol9", 4096, (void *)9, 4, NULL) != pdPASS
                           || xTaskCreate(udp_task, "wol7", 4096, (void *)7, 4, NULL) != pdPASS)) abort();
        if (configured) {
            ps5_state_start(wol_ps5_set ? wol_ps5 : NULL, network_up, power_changed); /* console on / rest / off */
            ha_mqtt_start(); /* Home Assistant (MQTT), if enabled in the panel */
        }
        if (!configured) {
            ESP_LOGW(TAG, "Not configured: setup panel opened automatically");
            portal_start();
        } else if (portal_always) {
            ESP_LOGW(TAG, "Panel kept on permanently on the home network (panel option)");
            portal_lan_start();
        }
        if (xTaskCreate(button_task, "button", 3072, NULL, 3, NULL) != pdPASS
            || xTaskCreate(wifi_watch_task, "wifi_watch", 2560, NULL, 2, NULL) != pdPASS) abort();
    }
    printf("PS5_WOL_READY configured=%d; wake on WoL/WAKE/short BOOT press; hold BOOT 5 s for the setup panel at https://192.168.4.1 (started automatically when not configured)\n",
           configured);
    console();
}
