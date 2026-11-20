/* Console power state through the PlayStation Device Discovery Protocol (DDP), as used by the PS app and Remote Play.
 *
 * A UDP broadcast "SRCH * HTTP/1.1" to port 9302 is answered by every PS5 on the network:
 *   "HTTP/1.1 200 Ok"              -> on
 *   "HTTP/1.1 620 Server Standby"  -> rest mode (only if the console stays connected to the network in rest mode)
 *   no answer for OFF_AFTER_S      -> off, or not reachable
 * Measured on a PS5: switched off from the menu it goes silent at once; going to rest mode it either answers
 * "standby" at once or is silent for a while (seen: 30+ s); in rest mode single answers get lost, and it sometimes
 * answers "on" for a moment (background tasks). Hence: two queries per poll, a change between on and standby only
 * after the same answer twice in a row, and "off" only after OFF_AFTER_S of silence.
 * If the PS5's network MAC is set in the panel, only that console counts (host-id is its MAC); otherwise the first
 * PS5 that answers. Polled every 10 s, every 2 s for a minute after a wake attempt. */
#include "ps5_state.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#define DDP_PORT 9302
#define OFF_AFTER_S 60

static const char *TAG = "ps5_state";
static volatile ps5_power_t power = PS5_POWER_UNKNOWN;
static char host_name[33], host_ip[16];
static uint8_t filter_mac[6];
static bool filter_set;
static volatile int64_t fast_until;
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static bool (*network_up)(void);
static void copy_field_ext(const char *reply, const char *key, char *out, size_t size)
{
    const char *at = strstr(reply, key);
    if (!at) return;
    at += strlen(key);
    size_t n = 0;
    while (at[n] && at[n] != '\r' && at[n] != '\n' && n + 1 < size) ++n;
    memcpy(out, at, n);
    out[n] = 0;
}

#define MAX_CONSOLES 4
static ps5_console_t consoles[MAX_CONSOLES];
static int64_t console_seen[MAX_CONSOLES];

/* Remember every PS5 that answers (for choosing the console in the panel); forgotten after 10 minutes of silence. */
static void remember(const char *reply, const struct sockaddr_in *from, ps5_power_t state)
{
    char id[13] = "";
    copy_field_ext(reply, "host-id:", id, sizeof(id));
    if (strlen(id) != 12) return;
    int slot = -1, oldest = 0;
    portENTER_CRITICAL(&lock);
    for (int i = 0; i < MAX_CONSOLES; ++i) {
        if (consoles[i].mac[0] && !strcasecmp(consoles[i].mac, id)) { slot = i; break; }
        if (!consoles[i].mac[0] && slot < 0) slot = i;
        if (console_seen[i] < console_seen[oldest]) oldest = i;
    }
    if (slot < 0) slot = oldest;
    ps5_console_t *c = &consoles[slot];
    memcpy(c->mac, id, sizeof(id));
    c->name[0] = 0;
    copy_field_ext(reply, "host-name:", c->name, sizeof(c->name));
    inet_ntoa_r(from->sin_addr, c->ip, sizeof(c->ip));
    c->state = state;
    console_seen[slot] = esp_timer_get_time();
    portEXIT_CRITICAL(&lock);
}

int ps5_state_list(ps5_console_t *out, int max)
{
    int count = 0;
    int64_t now = esp_timer_get_time();
    portENTER_CRITICAL(&lock);
    for (int i = 0; i < MAX_CONSOLES && count < max; ++i)
        if (consoles[i].mac[0] && now - console_seen[i] < 600LL * 1000000) out[count++] = consoles[i];
    portEXIT_CRITICAL(&lock);
    return count;
}

bool ps5_state_filtered(void) { return filter_set; }
static void (*changed_cb)(ps5_power_t);

const char *ps5_power_name(ps5_power_t state)
{
    switch (state) {
    case PS5_POWER_ON: return "on";
    case PS5_POWER_STANDBY: return "standby";
    case PS5_POWER_OFF: return "off";
    default: return "unknown";
    }
}

ps5_power_t ps5_state_get(char name[33], char ip[16])
{
    portENTER_CRITICAL(&lock);
    ps5_power_t state = power;
    if (name) memcpy(name, host_name, 33);
    if (ip) memcpy(ip, host_ip, 16);
    portEXIT_CRITICAL(&lock);
    return state;
}

void ps5_state_kick(void) { fast_until = esp_timer_get_time() + 60LL * 1000000; }

static void set_state(ps5_power_t state)
{
    if (state == power) return;
    power = state;
    ESP_LOGI(TAG, "PS5_POWER %s", ps5_power_name(state));
    if (changed_cb) changed_cb(state);
}

/* "host-id:" is the console's MAC as 12 hex digits. */
static bool host_matches(const char *reply)
{
    if (!filter_set) return true;
    const char *id = strstr(reply, "host-id:");
    if (!id) return false;
    char expected[13];
    snprintf(expected, sizeof(expected), "%02X%02X%02X%02X%02X%02X", filter_mac[0], filter_mac[1], filter_mac[2],
             filter_mac[3], filter_mac[4], filter_mac[5]);
    return !strncasecmp(id + 8, expected, 12);
}


static void state_task(void *arg)
{
    (void)arg;
    int sock = -1;
    int64_t last_seen = 0;
    int last_raw = -1; /* diagnostic: log every change of what the console answers (on / standby / silent) */
    ps5_power_t candidate = PS5_POWER_UNKNOWN;
    for (;;) {
        bool fast = esp_timer_get_time() < fast_until;
        vTaskDelay(pdMS_TO_TICKS(fast ? 2000 : 10000));
        if (network_up && !network_up()) continue;
        if (sock < 0) {
            sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock < 0) continue;
            int yes = 1;
            setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
            struct timeval timeout = {.tv_sec = 0, .tv_usec = 300000};
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        }
        static const char query[] = "SRCH * HTTP/1.1\ndevice-discovery-protocol-version:00030010\n";
        struct sockaddr_in to = {.sin_family = AF_INET, .sin_port = htons(DDP_PORT)};
        to.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        if (sendto(sock, query, sizeof(query) - 1, 0, (struct sockaddr *)&to, sizeof(to)) < 0) {
            close(sock);
            sock = -1;
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
        sendto(sock, query, sizeof(query) - 1, 0, (struct sockaddr *)&to, sizeof(to)); /* a second try: UDP gets lost */
        ps5_power_t found = PS5_POWER_UNKNOWN;
        int64_t until = esp_timer_get_time() + 1500000;
        char reply[512];
        while (esp_timer_get_time() < until) {
            struct sockaddr_in from;
            socklen_t from_len = sizeof(from);
            int length = recvfrom(sock, reply, sizeof(reply) - 1, 0, (struct sockaddr *)&from, &from_len);
            if (length <= 0) continue;
            reply[length] = 0;
            if (strncmp(reply, "HTTP/1.1 ", 9) || !strstr(reply, "host-type:PS5")) continue;
            ps5_power_t answer = !strncmp(reply + 9, "200", 3) ? PS5_POWER_ON
                               : !strncmp(reply + 9, "620", 3) ? PS5_POWER_STANDBY : PS5_POWER_UNKNOWN;
            if (answer == PS5_POWER_UNKNOWN) continue;
            remember(reply, &from, answer);
            if (found != PS5_POWER_UNKNOWN || !host_matches(reply)) continue; /* keep listening for other consoles */
            found = answer;
            portENTER_CRITICAL(&lock);
            host_name[0] = 0;
            copy_field_ext(reply, "host-name:", host_name, sizeof(host_name));
            inet_ntoa_r(from.sin_addr, host_ip, sizeof(host_ip));
            portEXIT_CRITICAL(&lock);
        }
        if ((int)found != last_raw) {
            last_raw = (int)found;
            ESP_LOGI(TAG, "PS5_DDP %s at %lld s", found == PS5_POWER_ON ? "answer=on" : found == PS5_POWER_STANDBY ? "answer=standby" : "silent",
                     esp_timer_get_time() / 1000000);
        }
        if (found != PS5_POWER_UNKNOWN) {
            last_seen = esp_timer_get_time();
            /* first sighting, or coming back from off: at once; between on and standby: same answer twice */
            if (power == PS5_POWER_UNKNOWN || power == PS5_POWER_OFF || found == candidate) set_state(found);
            candidate = found;
        } else if (power != PS5_POWER_UNKNOWN && power != PS5_POWER_OFF
                   && esp_timer_get_time() - last_seen > (int64_t)OFF_AFTER_S * 1000000) {
            set_state(PS5_POWER_OFF); /* seen before, silent for OFF_AFTER_S: off or left the network */
        }
        if (found == PS5_POWER_UNKNOWN) candidate = PS5_POWER_UNKNOWN; /* "twice in a row" means consecutive polls */
    }
}

void ps5_state_start(const uint8_t *ps5_mac, bool (*is_network_up)(void), void (*on_change)(ps5_power_t))
{
    if (ps5_mac) {
        memcpy(filter_mac, ps5_mac, 6);
        filter_set = true;
    }
    network_up = is_network_up;
    changed_cb = on_change;
    ps5_state_kick();
    if (xTaskCreate(state_task, "ps5_state", 3584, NULL, 2, NULL) != pdPASS) ESP_LOGE(TAG, "task not started");
}
