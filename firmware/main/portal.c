#include "portal.h"
#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "mdns.h"
#include "mbedtls/base64.h"
#include "panel_tls.h"

static const char *TAG = "portal";
static volatile bool active;      /* panel access point (192.168.4.1) is on */
static bool web_on, mdns_on;
static httpd_handle_t ssl_server; /* HTTPS server of the panel (NULL until the certificate is ready) */
static volatile bool tls_task_running, newcert_request;
static bool tls_task_created;
static volatile int64_t last_activity;
static char ap_ssid[33];
static esp_timer_handle_t restart_timer;

extern const char portal_html_start[] asm("_binary_portal_html_start");
extern const char portal_html_end[] asm("_binary_portal_html_end");
extern const char reader_cmd_start[] asm("_binary_ps5_wake_reader_cmd_start");
extern const char reader_cmd_end[] asm("_binary_ps5_wake_reader_cmd_end");
extern const char reader_py_start[] asm("_binary_ps5_wake_reader_py_start");
extern const char reader_py_end[] asm("_binary_ps5_wake_reader_py_end");

bool portal_active(void) { return active; }

static void touch(void) { last_activity = esp_timer_get_time(); }

/* Local IPv4 address a connection arrived on (0 if unknown). 192.168.4.1 = the panel access point. */
static uint32_t local_ipv4(int sockfd)
{
    struct sockaddr_storage local;
    socklen_t length = sizeof(local);
    if (getsockname(sockfd, (struct sockaddr *)&local, &length) != 0) return 0;
    if (local.ss_family == AF_INET) return ntohl(((struct sockaddr_in *)&local)->sin_addr.s_addr);
    if (local.ss_family == AF_INET6) {
        const uint8_t *b = ((const struct sockaddr_in6 *)&local)->sin6_addr.s6_addr;
        static const uint8_t mapped[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};
        if (memcmp(b, mapped, sizeof(mapped)) != 0) return 0;
        return ((uint32_t)b[12] << 24) | ((uint32_t)b[13] << 16) | ((uint32_t)b[14] << 8) | b[15];
    }
    return 0;
}

#define AP_ADDRESS 0xC0A80401u

static bool via_ap(httpd_req_t *req) { return local_ipv4(httpd_req_to_sockfd(req)) == AP_ADDRESS; }

static bool is_secure(httpd_req_t *req) { return ssl_server && req->handle == ssl_server; }

/* http://host/... -> https://host/ (panel network, or home network with HTTPS on). Only plain host names and addresses are echoed. */
static void redirect_https(httpd_req_t *req)
{
    char host[64] = "";
    if (httpd_req_get_hdr_value_str(req, "Host", host, sizeof(host)) != ESP_OK) host[0] = 0;
    char *colon = strrchr(host, ':');
    if (colon && !strchr(host, ']')) *colon = 0; /* drop :80 */
    for (const char *c = host; *c; ++c)
        if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '.' || *c == '-'))
            host[0] = 0;
    if (!host[0]) {
        char name[40];
        app_get_hostname(name);
        snprintf(host, sizeof(host), "%s.local", name);
    }
    char location[96];
    snprintf(location, sizeof(location), "https://%s/", host);
    httpd_resp_set_status(req, "301 Moved Permanently");
    httpd_resp_set_hdr(req, "Location", location);
    httpd_resp_send(req, NULL, 0);
}

/* Constant-time comparison of two NUL-terminated strings (length is not hidden). */
static bool same_secret(const char *a, const char *b)
{
    size_t la = strlen(a), lb = strlen(b);
    unsigned char diff = la != lb;
    for (size_t i = 0; i < la && i < lb; ++i) diff |= (unsigned char)(a[i] ^ b[i]);
    return diff == 0;
}

static void https_starting(httpd_req_t *req)
{
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_set_hdr(req, "Retry-After", "10");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "PS5 Wake: HTTPS is starting (creating the certificate), try again in a few seconds.");
}

/* Access rules.
 * - Panel access point: always HTTPS (plain HTTP only redirects); the Wi-Fi password protects the page.
 * - Home network (only with "Access from home network"): HTTPS unless switched off in the panel, and every request needs
 *   HTTP Basic authentication: the panel user name (default "admin") and the same panel password.
 * Returns false after sending the refusal / redirect. */
static bool authorized(httpd_req_t *req)
{
    if (via_ap(req)) {
        if (!ssl_server) {
            https_starting(req);
            return false;
        }
        if (!is_secure(req)) {
            redirect_https(req);
            return false;
        }
        return true;
    }
    if (!app_portal_always()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Panel not available on this network");
        return false;
    }
    if (app_https() && !ssl_server) { /* creating or replacing the certificate */
        https_starting(req);
        return false;
    }
    if (app_https() && !is_secure(req)) { /* plain HTTP from the home network only redirects */
        redirect_https(req);
        return false;
    }
    bool ok = false;
    char header[160];
    if (httpd_req_get_hdr_value_str(req, "Authorization", header, sizeof(header)) == ESP_OK
        && !strncmp(header, "Basic ", 6)) {
        unsigned char decoded[120];
        size_t length = 0;
        if (mbedtls_base64_decode(decoded, sizeof(decoded) - 1, &length, (const unsigned char *)header + 6,
                                  strlen(header + 6)) == 0) {
            decoded[length] = 0;
            char *colon = strchr((char *)decoded, ':');
            char password[65], expected_user[33];
            app_get_ap_password(password);
            app_get_panel_user(expected_user);
            if (colon) *colon = 0;
            bool user_ok = colon && same_secret((char *)decoded, expected_user);
            bool pass_ok = colon && strcmp(password, PORTAL_DEFAULT_AP_PASSWORD) != 0 && same_secret(colon + 1, password);
            ok = user_ok && pass_ok;
            memset(password, 0, sizeof(password));
        }
        memset(decoded, 0, sizeof(decoded));
        if (!ok) {
            ESP_LOGW(TAG, "PORTAL: wrong panel user name or password from the home network");
            vTaskDelay(pdMS_TO_TICKS(1000)); /* slow down guessing */
        }
    }
    memset(header, 0, sizeof(header));
    if (ok) return true;
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"PS5 Wake\", charset=\"UTF-8\"");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "PS5 Wake: enter the panel user name (default: admin) and the panel password.");
    return false;
}

static void restart_cb(void *arg)
{
    (void)arg;
    esp_restart();
}

static void restart_soon(void)
{
    const esp_timer_create_args_t args = {.callback = restart_cb, .name = "portal_restart"};
    if (!restart_timer && esp_timer_create(&args, &restart_timer) != ESP_OK) esp_restart();
    esp_timer_start_once(restart_timer, 1500000);
}

static esp_err_t send_json(httpd_req_t *req, cJSON *root)
{
    char *text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    esp_err_t result = httpd_resp_sendstr(req, text ? text : "{}");
    cJSON_free(text);
    return result;
}

/* Reads a small JSON body; NULL on error (response already sent). Caller frees with cJSON_Delete. */
static cJSON *read_json(httpd_req_t *req)
{
    char body[1024];
    if (req->content_len <= 0 || req->content_len >= (int)sizeof(body)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return NULL;
    }
    int received = 0;
    while (received < req->content_len) {
        int n = httpd_req_recv(req, body + received, req->content_len - received);
        if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (n <= 0) return NULL;
        received += n;
    }
    body[received] = 0;
    cJSON *root = cJSON_Parse(body);
    memset(body, 0, sizeof(body)); /* may contain passwords */
    if (!root) httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad JSON");
    return root;
}

static esp_err_t index_get(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    touch();
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, portal_html_start, portal_html_end - portal_html_start - 1);
}

/* Address reader downloads (Windows .cmd, Python for macOS/Linux), embedded in the firmware. */
static esp_err_t download_get(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    touch();
    bool windows = strstr(req->uri, ".cmd") != NULL;
    httpd_resp_set_type(req, windows ? "application/octet-stream" : "text/x-python");
    httpd_resp_set_hdr(req, "Content-Disposition", windows ? "attachment; filename=\"ps5-wake-reader.cmd\""
                                                           : "attachment; filename=\"ps5_wake_reader.py\"");
    const char *start = windows ? reader_cmd_start : reader_py_start;
    const char *end = windows ? reader_cmd_end : reader_py_end;
    return httpd_resp_send(req, start, end - start - 1);
}

/* "New certificate" button: the certificate task replaces it within a few seconds (HTTPS restarts). */
static esp_err_t newcert_post(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    touch();
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", tls_task_running);
    if (tls_task_running) newcert_request = true;
    return send_json(req, root);
}

/* The device certificate, for users who want to trust it in their system and get rid of the browser warning. */
static esp_err_t cert_get(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    const char *pem = panel_tls_cert_pem();
    if (!pem) return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No certificate yet");
    httpd_resp_set_type(req, "application/x-x509-ca-cert");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"ps5-wake.crt\"");
    return httpd_resp_sendstr(req, pem);
}

static esp_err_t status_get(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    cJSON *root = cJSON_CreateObject();
    app_status_json(root);
    int64_t left = (int64_t)PORTAL_IDLE_MINUTES * 60 - (esp_timer_get_time() - last_activity) / 1000000;
    cJSON_AddNumberToObject(root, "portal_seconds_left", left > 0 ? (double)left : 0);
    cJSON_AddStringToObject(root, "ap_ssid", ap_ssid);
    cJSON_AddBoolToObject(root, "via_ap", via_ap(req));
    cJSON_AddBoolToObject(root, "secure", is_secure(req));
    char fingerprint[96], expires[11];
    if (tls_task_running && panel_tls_info(fingerprint, expires)) {
        cJSON_AddStringToObject(root, "cert_sha256", fingerprint);
        cJSON_AddStringToObject(root, "cert_expires", expires);
        cJSON_AddBoolToObject(root, "clock", panel_tls_time_known());
    }
    return send_json(req, root);
}

static esp_err_t scan_get(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    touch();
    cJSON *root = cJSON_CreateObject();
    cJSON *networks = cJSON_AddArrayToObject(root, "networks");
    app_scan_json(networks);
    return send_json(req, root);
}

static esp_err_t save_post(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    touch();
    cJSON *body = read_json(req);
    if (!body) return ESP_OK;
    const char *error = NULL;
    esp_err_t result = app_save_json(body, &error);
    cJSON_Delete(body);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", result == ESP_OK);
    if (result != ESP_OK) cJSON_AddStringToObject(root, "error", error ? error : esp_err_to_name(result));
    esp_err_t sent = send_json(req, root);
    if (result == ESP_OK) {
        ESP_LOGW(TAG, "PORTAL: settings saved, restarting");
        restart_soon();
    }
    return sent;
}

static esp_err_t wake_post(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    touch();
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "accepted", app_request_wake());
    return send_json(req, root);
}

static esp_err_t lang_post(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    touch();
    cJSON *body = read_json(req);
    if (!body) return ESP_OK;
    const cJSON *lang = cJSON_GetObjectItemCaseSensitive(body, "lang");
    esp_err_t result = cJSON_IsString(lang) ? app_set_lang(lang->valuestring) : ESP_ERR_INVALID_ARG;
    cJSON_Delete(body);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", result == ESP_OK);
    return send_json(req, root);
}

/* "Extend" button: restarts the idle countdown (another PORTAL_IDLE_MINUTES). */
static esp_err_t extend_post(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    touch();
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    return send_json(req, root);
}

static esp_err_t exit_post(httpd_req_t *req)
{
    if (!authorized(req)) return ESP_OK;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    esp_err_t sent = send_json(req, root);
    if (via_ap(req)) { /* from the home network there is no access point to close */
        ESP_LOGW(TAG, "PORTAL: closed by user, restarting");
        restart_soon();
    }
    return sent;
}

/* Unknown paths go to the panel's start page. */
static esp_err_t redirect_404(httpd_req_t *req, httpd_err_code_t error)
{
    (void)error;
    if (!authorized(req)) return ESP_OK;
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

static void idle_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        if (!app_configured()) continue; /* a new device keeps its panel until it is set up */
        if (esp_timer_get_time() - last_activity > (int64_t)PORTAL_IDLE_MINUTES * 60 * 1000000) {
            ESP_LOGW(TAG, "PORTAL: %d min without activity, closing (restart)", PORTAL_IDLE_MINUTES);
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_restart();
        }
    }
}

/* Connections through the panel access point are accepted while it is on; connections from the home network only
 * with "Access from home network" (and then every request needs the panel password, see authorized()). */
static esp_err_t portal_session_open(httpd_handle_t handle, int sockfd)
{
    (void)handle;
    uint32_t address = local_ipv4(sockfd);
    if (address == AP_ADDRESS ? active : (address != 0 && app_portal_always())) return ESP_OK;
    ESP_LOGW(TAG, "PORTAL: refused a connection from a network where the panel is not available");
    return ESP_FAIL;
}

static void mdns_start(void)
{
    if (mdns_on) return;
    if (mdns_init() == ESP_OK) {
        char hostname[40], device[25], instance[40];
        app_get_hostname(hostname);
        app_get_device_name(device);
        snprintf(instance, sizeof(instance), "%s", device[0] ? device : "PS5 Wake");
        mdns_hostname_set(hostname);
        mdns_instance_name_set(instance);
        ESP_LOGW(TAG, "mDNS name: %s.local", hostname);
        mdns_service_add("PS5 Wake", "_https", "_tcp", 443, NULL, 0);
        mdns_on = true;
    } else ESP_LOGW(TAG, "mDNS not started");
}

static void register_routes(httpd_handle_t server)
{
    const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = index_get},
        {.uri = "/api/status", .method = HTTP_GET, .handler = status_get},
        {.uri = "/api/scan", .method = HTTP_GET, .handler = scan_get},
        {.uri = "/api/save", .method = HTTP_POST, .handler = save_post},
        {.uri = "/api/wake", .method = HTTP_POST, .handler = wake_post},
        {.uri = "/api/exit", .method = HTTP_POST, .handler = exit_post},
        {.uri = "/api/lang", .method = HTTP_POST, .handler = lang_post},
        {.uri = "/api/extend", .method = HTTP_POST, .handler = extend_post},
        {.uri = "/download/ps5-wake-reader.cmd", .method = HTTP_GET, .handler = download_get},
        {.uri = "/download/ps5_wake_reader.py", .method = HTTP_GET, .handler = download_get},
        {.uri = "/download/ps5-wake.crt", .method = HTTP_GET, .handler = cert_get},
        {.uri = "/api/newcert", .method = HTTP_POST, .handler = newcert_post},
    };
    for (unsigned i = 0; i < sizeof(routes) / sizeof(routes[0]); ++i) httpd_register_uri_handler(server, &routes[i]);
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, redirect_404);
}

static bool web_start(void)
{
    if (web_on) return true;
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 14;
    config.max_open_sockets = 4;
    config.lru_purge_enable = true;
    config.stack_size = 6144;
    config.open_fn = portal_session_open;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "PORTAL: web server did not start");
        return false;
    }
    register_routes(server);
    web_on = true;
    return true;
}

/* "Access from home network": the page is served on the home network (the address from DHCP, and ps5-wake-xxxx.local) behind
 * the panel password. The panel access point is not started. */
static void ssl_start(void)
{
    if (ssl_server) return;
    const char *cert, *key;
    size_t cert_len, key_len;
    if (!panel_tls_get(&cert, &cert_len, &key, &key_len)) {
        ESP_LOGE(TAG, "PANEL_LAN: no certificate, HTTPS not started");
        return;
    }
    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
    config.servercert = (const uint8_t *)cert;
    config.servercert_len = cert_len;
    config.prvtkey_pem = (const uint8_t *)key;
    config.prvtkey_len = key_len;
    config.port_secure = 443;
    config.httpd.ctrl_port = ESP_HTTPD_DEF_CTRL_PORT + 1; /* the plain server uses the default one */
    config.httpd.max_uri_handlers = 14;
    config.httpd.max_open_sockets = 2; /* each TLS session needs tens of kB of RAM */
    config.httpd.lru_purge_enable = true;
    config.httpd.stack_size = 10240;
    config.httpd.open_fn = portal_session_open;
    httpd_handle_t server = NULL;
    if (httpd_ssl_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "PANEL_LAN: HTTPS server did not start");
        return;
    }
    register_routes(server);
    ssl_server = server;
}

static void ssl_stop(void)
{
    httpd_handle_t server = ssl_server;
    ssl_server = NULL;
    if (server) httpd_ssl_stop(server);
}

/* Makes sure a certificate exists and runs the HTTPS server; gets the time from NTP (for certificate dates) when
 * the device is connected to the home network; checks the certificate every hour and renews it before it expires,
 * as soon as the time becomes known (replacing the no-clock certificate), or when the panel asks for a new one. */
static void tls_task(void *arg)
{
    (void)arg;
    bool changed;
    char fingerprint[96], expires[11];
    panel_tls_ensure(false, &changed); /* a new device: a few seconds; otherwise the stored certificate */
    ssl_start();
    tls_task_running = true;
    if (panel_tls_info(fingerprint, expires))
        ESP_LOGW(TAG, "PANEL_TLS: certificate SHA-256 %s, valid until %s", fingerprint, expires);
    if (app_configured()) {
        esp_sntp_config_t sntp = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        esp_netif_sntp_init(&sntp);
    }
    bool time_known = panel_tls_time_known();
    int64_t last_check = esp_timer_get_time();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        bool force = newcert_request;
        bool clock_set = !time_known && panel_tls_time_known();
        if (clock_set) {
            time_known = true;
            ESP_LOGW(TAG, "PANEL_TLS: time received from NTP");
        }
        if (!force && !clock_set && esp_timer_get_time() - last_check < 3600LL * 1000000) continue;
        last_check = esp_timer_get_time();
        newcert_request = false;
        if (!force && !panel_tls_due()) continue;
        if (force) vTaskDelay(pdMS_TO_TICKS(1000)); /* let the button's response leave first */
        ssl_stop();
        panel_tls_ensure(force, &changed);
        ssl_start();
        ESP_LOGW(TAG, "PANEL_TLS: certificate %s, HTTPS %s", changed ? "replaced" : "unchanged",
                 ssl_server ? "running" : "NOT running");
        if (changed && panel_tls_info(fingerprint, expires))
            ESP_LOGW(TAG, "PANEL_TLS: certificate SHA-256 %s, valid until %s", fingerprint, expires);
    }
}

static void tls_task_start(void)
{
    if (tls_task_created) return;
    tls_task_created = xTaskCreate(tls_task, "panel_tls", 8192, NULL, 2, NULL) == pdPASS;
    if (!tls_task_created) ESP_LOGE(TAG, "PANEL_TLS: certificate task not started");
}

void portal_lan_start(void)
{
    mdns_start();
    if (!web_start()) return;
    if (app_https()) tls_task_start();
    ESP_LOGW(TAG, "PANEL_LAN: panel available on the home network (%s, password protected)",
             app_https() ? "HTTPS" : "HTTP");
}

void portal_start(void)
{
    if (active) {
        touch();
        return;
    }
    touch();
    app_portal_pause_sta(); /* a reconnecting station would hop channels and disturb the access point */
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    /* No captive portal: the panel does not pop up by itself; the user opens https://192.168.4.1/ . */
    (void)ap_netif;
    /* ps5-wake-xxxx.local: reachable by name even when the computer also has another network (cable, other Wi-Fi)
     * and asks its other DNS server first. Started before the access point so it sees the AP start. */
    mdns_start();
    if (esp_wifi_set_mode(WIFI_MODE_APSTA) != ESP_OK) {
        ESP_LOGE(TAG, "PORTAL: cannot enable access point");
        return;
    }
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(ap_ssid, sizeof(ap_ssid), "PS5-WAKE-%02X%02X", mac[4], mac[5]);
    char password[65];
    app_get_ap_password(password);
    wifi_config_t ap = {0};
    memcpy(ap.ap.ssid, ap_ssid, strlen(ap_ssid));
    ap.ap.ssid_len = strlen(ap_ssid);
    memcpy(ap.ap.password, password, strlen(password));
    memset(password, 0, sizeof(password));
    ap.ap.channel = 1;
    wifi_ap_record_t current;
    if (esp_wifi_sta_get_ap_info(&current) == ESP_OK) ap.ap.channel = current.primary;
    ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap.ap.max_connection = 2;
    ap.ap.pmf_cfg.required = false;
    esp_err_t result = esp_wifi_set_config(WIFI_IF_AP, &ap);
    memset(&ap, 0, sizeof(ap));
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "PORTAL: access point config rejected: %s", esp_err_to_name(result));
        return;
    }

    active = true; /* before the server accepts the first connection through the access point */
    if (!web_start()) {
        active = false;
        return;
    }
    tls_task_start();
    xTaskCreate(idle_task, "portal_idle", 2048, NULL, 2, NULL);
    char hostname[40];
    app_get_hostname(hostname);
    ESP_LOGW(TAG, "PORTAL_ACTIVE ssid=%s url=https://192.168.4.1/ or https://%s.local/ (closes after %d min idle)",
             ap_ssid, hostname, PORTAL_IDLE_MINUTES);
}
