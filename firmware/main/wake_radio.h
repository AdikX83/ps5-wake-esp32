#pragma once
#include <stdint.h>
#include "esp_err.h"
#include <stdbool.h>
/* derived_identity=true: diagnostic only. Uses an address the C6 radio can receive with (adjust=1);
 * the PS5 does not know it, so it cannot wake the console. */
esp_err_t wake_radio_attempt(const uint8_t ps5[6], const uint8_t controller[6], bool derived_identity);
/* Result of the last attempt for the configuration panel: conn_status -1 = no attempt yet,
 * -2 = no Connection Complete, otherwise the HCI status (0 = link up). */
void wake_radio_last(int *conn_status, uint16_t *lmp_rx, int64_t *time_us);
