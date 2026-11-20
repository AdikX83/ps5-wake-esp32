#include "led.h"
#include <stdint.h>
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO 8

static const char *TAG = "led";
static rmt_channel_handle_t channel;
static rmt_encoder_handle_t encoder;
static volatile bool busy;
static volatile bool dark; /* "Turn the LED off completely": only the BOOT hold feedback is shown */
static volatile int hold;
static volatile led_event_t pending_event;
static volatile bool event_pending;

typedef struct { uint8_t r, g, b; } rgb_t;
static const rgb_t OFF = {0, 0, 0};
static const rgb_t PURPLE = {28, 0, 36}, BLUE = {0, 0, 48}, ORANGE = {48, 14, 0}, DIM_GREEN = {0, 8, 0},
                   RED = {48, 0, 0}, YELLOW = {36, 28, 0}, WHITE = {32, 32, 32}, CYAN = {0, 36, 36},
                   GREEN = {0, 48, 0};

static void show(rgb_t c)
{
    static rgb_t last = {1, 2, 3};
    if (!channel || (c.r == last.r && c.g == last.g && c.b == last.b)) return;
    last = c;
    const uint8_t grb[3] = {c.g, c.r, c.b}; /* WS2812 takes green, red, blue */
    const rmt_transmit_config_t config = {.loop_count = 0};
    if (rmt_transmit(channel, encoder, grb, sizeof(grb), &config) == ESP_OK) rmt_tx_wait_all_done(channel, 20);
}

void led_event(led_event_t event)
{
    pending_event = event;
    event_pending = true;
}

void led_set_busy(bool value) { busy = value; }
void led_set_dark(bool value) { dark = value; }
void led_set_hold(int level) { hold = level; }

/* Plays one event pattern (blocking, at most ~1 s). */
static void play_event(led_event_t event)
{
    rgb_t colour = WHITE;
    int flashes = 1, on_ms = 120, off_ms = 120;
    switch (event) {
    case LED_EVENT_TRIGGER: colour = WHITE; flashes = 1; on_ms = 150; break;
    case LED_EVENT_SUCCESS: colour = GREEN; flashes = 3; break;
    case LED_EVENT_FAILURE: colour = RED; flashes = 3; break;
    case LED_EVENT_IGNORED: colour = ORANGE; flashes = 1; on_ms = 150; break;
    }
    for (int i = 0; i < flashes; ++i) {
        show(colour);
        vTaskDelay(pdMS_TO_TICKS(on_ms));
        show(OFF);
        vTaskDelay(pdMS_TO_TICKS(off_ms));
    }
}

static void led_task(void *arg)
{
    (void)arg;
    for (;;) {
        int64_t ms = esp_timer_get_time() / 1000;
        if (hold == 4) show((ms / 100) % 2 ? PURPLE : OFF);   /* factory reset */
        else if (hold == 3) show((ms / 100) % 2 ? RED : OFF); /* reset is near */
        else if (hold == 2) show(RED);
        else if (hold == 1) show(WHITE);
        else if (dark) {
            event_pending = false;
            show(OFF);
        } else if (event_pending) {
            event_pending = false;
            play_event(pending_event);
            continue;
        } else if (busy) show((ms / 100) % 2 ? CYAN : OFF); /* 5 blinks per second */
        else {
            bool slow_on = (ms / 500) % 2 == 0; /* 1 blink per second */
            switch (app_led_base()) {
            case LED_BASE_UNCONFIGURED: show(PURPLE); break;
            case LED_BASE_CONNECTING: show(slow_on ? BLUE : OFF); break;
            case LED_BASE_NO_WIFI: show(slow_on ? ORANGE : OFF); break;
            case LED_BASE_READY: show(DIM_GREEN); break;
            case LED_BASE_PORTAL: show(RED); break;
            case LED_BASE_SAFE: show(YELLOW); break;
            default: show(OFF); break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void led_init(void)
{
    const rmt_tx_channel_config_t config = {
        .gpio_num = LED_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000, /* 0.1 us per tick */
        .mem_block_symbols = 48,
        .trans_queue_depth = 2,
    };
    const rmt_bytes_encoder_config_t bits = {
        .bit0 = {.level0 = 1, .duration0 = 3, .level1 = 0, .duration1 = 9}, /* 0.3 us high, 0.9 us low */
        .bit1 = {.level0 = 1, .duration0 = 9, .level1 = 0, .duration1 = 3}, /* 0.9 us high, 0.3 us low */
        .flags.msb_first = 1,
    };
    if (rmt_new_tx_channel(&config, &channel) != ESP_OK || rmt_new_bytes_encoder(&bits, &encoder) != ESP_OK
        || rmt_enable(channel) != ESP_OK) {
        ESP_LOGW(TAG, "Status LED unavailable");
        channel = NULL;
        return;
    }
    xTaskCreate(led_task, "led", 2560, NULL, 2, NULL);
}
