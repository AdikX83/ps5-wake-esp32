#pragma once
/* Status LED: the WS2812 RGB LED of the ESP32-C6-DevKitM-1 on GPIO8.
 * Steady colours show the mode (from app_led_base), blinks and flashes show events. */
#include <stdbool.h>

typedef enum {
    LED_BASE_OFF,
    LED_BASE_UNCONFIGURED, /* purple, steady */
    LED_BASE_CONNECTING,   /* blue, slow blink */
    LED_BASE_NO_WIFI,      /* orange, slow blink */
    LED_BASE_READY,        /* dim green, steady (or off when "LED off when ready" is set) */
    LED_BASE_PORTAL,       /* red, steady */
    LED_BASE_SAFE,         /* yellow, steady */
} led_base_t;

typedef enum {
    LED_EVENT_TRIGGER, /* one white flash: WoL packet or short BOOT press */
    LED_EVENT_SUCCESS, /* three green flashes: the PS5 answered */
    LED_EVENT_FAILURE, /* three red flashes: the PS5 did not answer */
    LED_EVENT_IGNORED, /* one orange flash: attempt running or 30 s cooldown */
} led_event_t;

void led_init(void);
void led_event(led_event_t event);
void led_set_busy(bool busy);   /* cyan fast blink while a wake attempt runs */
void led_set_dark(bool dark);   /* everything off except the BOOT hold feedback */
void led_set_hold(int level);   /* BOOT held: 0 none, 1 white 1-5 s, 2 red 5-15 s, 3 red blink 15-20 s, 4 purple blink: reset */

/* Provided by main.c. */
led_base_t app_led_base(void);
