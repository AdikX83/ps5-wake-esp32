#pragma once
/* Configuration panel.
 * portal_start(): Wi-Fi access point PS5-WAKE-xxxx + web server, started by releasing BOOT after 5-20 s or
 *   automatically on an unconfigured device. The access point stops only by restarting the ESP32: after Save, after
 *   "Close", or after PORTAL_IDLE_MINUTES without a user action (not while the device is unconfigured).
 * portal_lan_start(): "Access from home network" - the same page on the home network (DHCP address, ps5-wake-xxxx.local),
 *   every request protected by HTTP Basic authentication with the panel password; no access point.
 *   With HTTPS on (default) it uses a self-signed device certificate and plain HTTP only redirects. */
#include <stdbool.h>
#include "cJSON.h"
#include "esp_err.h"

#define PORTAL_IDLE_MINUTES 10
#define PORTAL_DEFAULT_AP_PASSWORD "ps5wol-setup"
#define PORTAL_DEFAULT_USER "admin" /* panel user name on the home network (HTTP Basic), changeable */

void portal_start(void);
void portal_lan_start(void);
bool portal_active(void);

/* Provided by main.c. */
void app_status_json(cJSON *root);
void app_scan_json(cJSON *networks);
esp_err_t app_save_json(const cJSON *root, const char **error);
bool app_request_wake(void);
void app_get_ap_password(char out[65]);
void app_get_panel_user(char out[33]);
void app_get_device_name(char out[25]);
void app_get_hostname(char out[40]); /* without ".local" */
void app_portal_pause_sta(void);
bool app_portal_always(void);
bool app_configured(void);
bool app_https(void); /* HTTPS for the home-network panel (default on) */
void app_get_lang(char out[8]);
esp_err_t app_set_lang(const char *code);
