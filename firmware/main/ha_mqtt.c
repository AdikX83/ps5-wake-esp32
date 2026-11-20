/* Home Assistant integration over MQTT with discovery.
 *
 * When enabled in the panel, the ESP32 connects to an MQTT broker (e.g. the Mosquitto add-on of Home Assistant) and
 * announces itself through Home Assistant MQTT discovery as a device "PS5 Wake" with:
 *   button  "Wake"            command  ps5wake/<id>/wake        (any payload)
 *   sensor  "Last wake"       state    ps5wake/<id>/result      idle | waking | answered | no_answer
 *   sensor  "Console"         state    ps5wake/<id>/power       on | standby | off | unknown
 *   sensor  "Wi-Fi signal"    state    ps5wake/<id>/rssi        dBm, every 60 s (diagnostic)
 *   availability              ps5wake/<id>/availability         online | offline (last will)
 * <id> is "ps5wake_" + the ESP32's Wi-Fi MAC in lower-case hex. Discovery is sent again when Home Assistant
 * restarts (homeassistant/status = online). */
#include "ha_mqtt.h"
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "portal.h"

static const char *TAG = "ha_mqtt";

static bool enabled;
static char url[129], user[65], pass[65];
static char id[24], base[40];
static esp_mqtt_client_handle_t client;
static volatile bool connected;
static char last_state[12] = "idle";
static char power_state[10] = "unknown";
static esp_timer_handle_t rssi_timer;

/* ---------- settings (NVS namespace "ps5_wol") ---------- */

static void get_str(nvs_handle_t handle, const char *key, char *out, size_t size)
{
    size_t length = size;
    if (nvs_get_str(handle, key, out, &length) != ESP_OK) out[0] = 0;
}

void ha_mqtt_load(void)
{
    nvs_handle_t handle;
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) != ESP_OK) return;
    uint8_t flag = 0;
    enabled = nvs_get_u8(handle, "mqtt_on", &flag) == ESP_OK && flag;
    get_str(handle, "mqtt_url", url, sizeof(url));
    get_str(handle, "mqtt_user", user, sizeof(user));
    get_str(handle, "mqtt_pass", pass, sizeof(pass));
    nvs_close(handle);
}

static bool url_valid(const char *text)
{
    size_t length = strlen(text);
    if (length < 9 || length >= sizeof(url)) return false;
    if (strncmp(text, "mqtt://", 7) && strncmp(text, "mqtts://", 8)) return false;
    for (const char *c = text; *c; ++c)
        if (*c <= ' ' || *c == '"' || *c == '\\') return false;
    return true;
}

esp_err_t ha_mqtt_check(const cJSON *root, const char **error)
{
    const cJSON *on = cJSON_GetObjectItemCaseSensitive(root, "mqtt_on");
    const cJSON *new_url = cJSON_GetObjectItemCaseSensitive(root, "mqtt_url");
    const cJSON *new_user = cJSON_GetObjectItemCaseSensitive(root, "mqtt_user");
    const cJSON *new_pass = cJSON_GetObjectItemCaseSensitive(root, "mqtt_pass");
    if (!cJSON_IsTrue(on)) return ESP_OK;
    if (!cJSON_IsString(new_url) || !url_valid(new_url->valuestring)) {
        *error = "mqtt";
        return ESP_ERR_INVALID_ARG;
    }
    if ((cJSON_IsString(new_user) && strlen(new_user->valuestring) >= sizeof(user))
        || (cJSON_IsString(new_pass) && strlen(new_pass->valuestring) >= sizeof(pass))) {
        *error = "mqtt";
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t ha_mqtt_save(nvs_handle_t handle, const cJSON *root)
{
    const cJSON *on = cJSON_GetObjectItemCaseSensitive(root, "mqtt_on");
    const cJSON *new_url = cJSON_GetObjectItemCaseSensitive(root, "mqtt_url");
    const cJSON *new_user = cJSON_GetObjectItemCaseSensitive(root, "mqtt_user");
    const cJSON *new_pass = cJSON_GetObjectItemCaseSensitive(root, "mqtt_pass");
    esp_err_t result = nvs_set_u8(handle, "mqtt_on", cJSON_IsTrue(on) ? 1 : 0);
    if (result == ESP_OK && cJSON_IsString(new_url)) result = nvs_set_str(handle, "mqtt_url", new_url->valuestring);
    if (result == ESP_OK && cJSON_IsString(new_user)) result = nvs_set_str(handle, "mqtt_user", new_user->valuestring);
    /* An empty password field keeps the stored one; "mqtt_pass_clear" removes it. */
    if (result == ESP_OK && cJSON_IsString(new_pass) && new_pass->valuestring[0])
        result = nvs_set_str(handle, "mqtt_pass", new_pass->valuestring);
    if (result == ESP_OK && cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "mqtt_pass_clear"))) {
        result = nvs_erase_key(handle, "mqtt_pass");
        if (result == ESP_ERR_NVS_NOT_FOUND) result = ESP_OK;
    }
    return result;
}

void ha_mqtt_status_json(cJSON *root)
{
    cJSON_AddBoolToObject(root, "mqtt_on", enabled);
    cJSON_AddStringToObject(root, "mqtt_url", url);
    cJSON_AddStringToObject(root, "mqtt_user", user);
    cJSON_AddBoolToObject(root, "mqtt_pass_set", pass[0] != 0);
    cJSON_AddBoolToObject(root, "mqtt_connected", connected);
    cJSON_AddStringToObject(root, "mqtt_id", id);
}

/* ---------- MQTT ---------- */

static void topic(char *out, size_t size, const char *leaf) { snprintf(out, size, "%s/%s", base, leaf); }

static void publish(const char *leaf, const char *payload, bool retain)
{
    if (!client) return;
    char name[64];
    topic(name, sizeof(name), leaf);
    esp_mqtt_client_enqueue(client, name, payload, 0, 1, retain, true);
}

static cJSON *device_json(void)
{
    cJSON *device = cJSON_CreateObject();
    cJSON *ids = cJSON_AddArrayToObject(device, "identifiers");
    cJSON_AddItemToArray(ids, cJSON_CreateString(id));
    char device_name[25], label[40];
    app_get_device_name(device_name);
    snprintf(label, sizeof(label), "%s", device_name[0] ? device_name : "PS5 Wake");
    cJSON_AddStringToObject(device, "name", label);
    cJSON_AddStringToObject(device, "manufacturer", "ps5-wake-esp32");
    cJSON_AddStringToObject(device, "model", "ESP32-C6");
    cJSON_AddStringToObject(device, "sw_version", esp_app_get_description()->version);
    esp_netif_ip_info_t ip = {0};
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta && esp_netif_get_ip_info(sta, &ip) == ESP_OK && ip.ip.addr && app_portal_always()) {
        char link[48];
        snprintf(link, sizeof(link), "%s://" IPSTR "/", app_https() ? "https" : "http", IP2STR(&ip.ip));
        cJSON_AddStringToObject(device, "configuration_url", link);
    }
    return device;
}

static void discovery_entity(const char *component, const char *object, cJSON *config)
{
    char name[48], value[64];
    snprintf(value, sizeof(value), "%s_%s", id, object);
    cJSON_AddStringToObject(config, "unique_id", value);
    /* No object_id: Home Assistant builds the entity ID from the device name and the entity name
     * (e.g. button.living_room_ps5_wake), adding _2, _3… if two devices have the same name. */
    topic(value, sizeof(value), "availability");
    cJSON_AddStringToObject(config, "availability_topic", value);
    cJSON_AddItemToObject(config, "device", device_json());
    char *text = cJSON_PrintUnformatted(config);
    cJSON_Delete(config);
    if (!text) return;
    char discovery[96];
    snprintf(name, sizeof(name), "%s", object);
    snprintf(discovery, sizeof(discovery), "homeassistant/%s/%s/%s/config", component, id, name);
    esp_mqtt_client_enqueue(client, discovery, text, 0, 1, 1, true);
    cJSON_free(text);
}

static void send_discovery(void)
{
    char value[64];
    cJSON *button = cJSON_CreateObject();
    cJSON_AddStringToObject(button, "name", "Wake");
    topic(value, sizeof(value), "wake");
    cJSON_AddStringToObject(button, "command_topic", value);
    cJSON_AddStringToObject(button, "payload_press", "PRESS");
    cJSON_AddStringToObject(button, "icon", "mdi:power");
    discovery_entity("button", "wake", button);

    cJSON *result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "name", "Last wake");
    topic(value, sizeof(value), "result");
    cJSON_AddStringToObject(result, "state_topic", value);
    cJSON_AddStringToObject(result, "device_class", "enum");
    cJSON *options = cJSON_AddArrayToObject(result, "options");
    for (const char *const *o = (const char *const[]){"idle", "waking", "answered", "no_answer", NULL}; *o; ++o)
        cJSON_AddItemToArray(options, cJSON_CreateString(*o));
    cJSON_AddStringToObject(result, "icon", "mdi:gamepad-variant");
    discovery_entity("sensor", "result", result);

    cJSON *power = cJSON_CreateObject();
    cJSON_AddStringToObject(power, "name", "Console");
    topic(value, sizeof(value), "power");
    cJSON_AddStringToObject(power, "state_topic", value);
    cJSON_AddStringToObject(power, "device_class", "enum");
    cJSON *states = cJSON_AddArrayToObject(power, "options");
    for (const char *const *o = (const char *const[]){"on", "standby", "off", "unknown", NULL}; *o; ++o)
        cJSON_AddItemToArray(states, cJSON_CreateString(*o));
    cJSON_AddStringToObject(power, "icon", "mdi:sony-playstation");
    discovery_entity("sensor", "power", power);

    cJSON *rssi = cJSON_CreateObject();
    cJSON_AddStringToObject(rssi, "name", "Wi-Fi signal");
    topic(value, sizeof(value), "rssi");
    cJSON_AddStringToObject(rssi, "state_topic", value);
    cJSON_AddStringToObject(rssi, "device_class", "signal_strength");
    cJSON_AddStringToObject(rssi, "unit_of_measurement", "dBm");
    cJSON_AddStringToObject(rssi, "state_class", "measurement");
    cJSON_AddStringToObject(rssi, "entity_category", "diagnostic");
    discovery_entity("sensor", "rssi", rssi);
}

static void publish_rssi(void *arg)
{
    (void)arg;
    if (!connected) return;
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) return;
    char text[8];
    snprintf(text, sizeof(text), "%d", ap.rssi);
    publish("rssi", text, false);
}

static void on_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_base;
    esp_mqtt_event_handle_t event = event_data;
    char name[64];
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        connected = true;
        ESP_LOGW(TAG, "MQTT: connected, announcing to Home Assistant as %s", id);
        topic(name, sizeof(name), "wake");
        esp_mqtt_client_subscribe(client, name, 1);
        esp_mqtt_client_subscribe(client, "homeassistant/status", 1);
        send_discovery();
        publish("availability", "online", true);
        publish("result", last_state, true);
        publish("power", power_state, true);
        publish_rssi(NULL);
        break;
    case MQTT_EVENT_DISCONNECTED:
        if (connected) ESP_LOGW(TAG, "MQTT: disconnected (reconnecting automatically)");
        connected = false;
        break;
    case MQTT_EVENT_DATA:
        topic(name, sizeof(name), "wake");
        if (event->topic_len == (int)strlen(name) && !memcmp(event->topic, name, event->topic_len)) {
            bool accepted = app_request_wake();
            ESP_LOGW(TAG, "MQTT: wake command from Home Assistant %s", accepted ? "accepted" : "ignored (busy or < 30 s)");
        } else if (event->topic_len == 20 && !memcmp(event->topic, "homeassistant/status", 20)
                   && event->data_len == 6 && !memcmp(event->data, "online", 6)) {
            send_discovery(); /* Home Assistant restarted */
            publish("availability", "online", true);
            publish("result", last_state, true);
            publish("power", power_state, true);
        }
        break;
    case MQTT_EVENT_ERROR:
        if (event->error_handle && event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
            ESP_LOGW(TAG, "MQTT: broker refused the connection (user name / password?) code=%d",
                     event->error_handle->connect_return_code);
        break;
    default:
        break;
    }
}

void ha_mqtt_start(void)
{
    if (!enabled || client || !url_valid(url)) return;
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(id, sizeof(id), "ps5wake_%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(base, sizeof(base), "ps5wake/%s", id);
    static char will_topic[64];
    topic(will_topic, sizeof(will_topic), "availability");
    esp_mqtt_client_config_t config = {
        .broker.address.uri = url,
        .credentials.client_id = id,
        .credentials.username = user[0] ? user : NULL,
        .credentials.authentication.password = pass[0] ? pass : NULL,
        .session.last_will = {.topic = will_topic, .msg = "offline", .qos = 1, .retain = 1},
        .session.keepalive = 60,
        .network.reconnect_timeout_ms = 10000,
        .task.stack_size = 6144,
        .buffer.size = 1024,
    };
    if (!strncmp(url, "mqtts://", 8)) config.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
    client = esp_mqtt_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "MQTT: client not created");
        return;
    }
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, on_event, NULL);
    if (esp_mqtt_client_start(client) != ESP_OK) ESP_LOGE(TAG, "MQTT: client not started");
    const esp_timer_create_args_t timer = {.callback = publish_rssi, .name = "mqtt_rssi"};
    if (esp_timer_create(&timer, &rssi_timer) == ESP_OK) esp_timer_start_periodic(rssi_timer, 60LL * 1000000);
    ESP_LOGW(TAG, "MQTT: Home Assistant integration enabled (%s)", url);
}

void ha_mqtt_wake_state(const char *state)
{
    snprintf(last_state, sizeof(last_state), "%s", state);
    if (connected) publish("result", state, true);
}

void ha_mqtt_power_state(const char *state)
{
    snprintf(power_state, sizeof(power_state), "%s", state);
    if (connected) publish("power", state, true);
}
