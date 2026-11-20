#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Console power state from the PlayStation discovery protocol (see ps5_state.c). */
typedef enum { PS5_POWER_UNKNOWN = 0, PS5_POWER_ON, PS5_POWER_STANDBY, PS5_POWER_OFF } ps5_power_t;

/* ps5_mac: the console's network MAC to match, or NULL for the first PS5 that answers. */
void ps5_state_start(const uint8_t *ps5_mac, bool (*is_network_up)(void), void (*on_change)(ps5_power_t));
ps5_power_t ps5_state_get(char name[33], char ip[16]); /* name/ip may be NULL */
void ps5_state_kick(void);                              /* poll quickly for a minute (after a wake attempt) */
const char *ps5_power_name(ps5_power_t state);          /* on | standby | off | unknown */

typedef struct {
    char mac[13];  /* host-id: network MAC, 12 hex digits */
    char name[33];
    char ip[16];
    ps5_power_t state;
} ps5_console_t;
int ps5_state_list(ps5_console_t *out, int max); /* consoles that answered in the last 10 minutes */
bool ps5_state_filtered(void);                   /* a specific console (PS5 MAC from the panel) is followed */
