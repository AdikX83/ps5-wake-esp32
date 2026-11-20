#pragma once
#include "cJSON.h"
#include "esp_err.h"
#include "nvs.h"

/* Home Assistant integration over MQTT discovery (see ha_mqtt.c). */
void ha_mqtt_load(void);                                       /* read settings from NVS */
esp_err_t ha_mqtt_check(const cJSON *root, const char **error); /* validate panel input before saving */
esp_err_t ha_mqtt_save(nvs_handle_t handle, const cJSON *root);  /* store panel input (inside the settings save) */
void ha_mqtt_status_json(cJSON *root);                          /* panel status fields */
void ha_mqtt_start(void);                                       /* connect if enabled; reconnects by itself */
void ha_mqtt_wake_state(const char *state);                     /* idle | waking | answered | no_answer */
void ha_mqtt_power_state(const char *state);                    /* on | standby | off | unknown */
