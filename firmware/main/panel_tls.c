/* Self-signed TLS certificate for the home-network panel (HTTPS).
 *
 * Every device generates its own EC P-256 key; browsers show a warning because no public authority signs it.
 * The certificate is stored in NVS (a factory reset erases it).
 * - With the real time from NTP: valid from yesterday for PANEL_TLS_DAYS days (Apple platforms reject longer TLS
 *   certificates), renewed automatically PANEL_TLS_RENEW_DAYS before it expires.
 * - Without the time (no internet yet, e.g. a new device in the panel network): the same 825 days counted from the
 *   firmware build date, replaced by a properly dated one as soon as NTP gives the time. */
#include "panel_tls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_random.h"
#include "mbedtls/ecp.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha256.h"
#include "mbedtls/version.h"
#include "mbedtls/x509_crt.h"
#include "nvs.h"
#include "portal.h"

#define PANEL_TLS_DAYS 825
#define PANEL_TLS_RENEW_DAYS 30

static const char *TAG = "panel_tls";
static char *cert_pem, *key_pem;

static int rng(void *ctx, unsigned char *out, size_t length)
{
    (void)ctx;
    esp_fill_random(out, length);
    return 0;
}

bool panel_tls_time_known(void)
{
    return time(NULL) > 1735689600; /* after 2025-01-01: the clock was set by NTP */
}

static char *nvs_read(nvs_handle_t handle, const char *key)
{
    size_t size = 0;
    if (nvs_get_str(handle, key, NULL, &size) != ESP_OK || size < 2) return NULL;
    char *text = malloc(size);
    if (text && nvs_get_str(handle, key, text, &size) != ESP_OK) {
        free(text);
        text = NULL;
    }
    return text;
}

static void unload(void)
{
    if (key_pem) memset(key_pem, 0, strlen(key_pem));
    free(cert_pem);
    free(key_pem);
    cert_pem = key_pem = NULL;
}

static bool load(void)
{
    if (cert_pem && key_pem) return true;
    unload();
    nvs_handle_t handle;
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) != ESP_OK) return false;
    cert_pem = nvs_read(handle, "tls_crt");
    key_pem = nvs_read(handle, "tls_key");
    nvs_close(handle);
    if (cert_pem && key_pem) return true;
    unload();
    return false;
}

/* Firmware build date (UTC midnight) - the best guess for "now" without a clock. */
static time_t build_time(void)
{
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *date = esp_app_get_description()->date; /* "Nov 19 2026" */
    char month[4] = {0};
    int day = 1, year = 2026;
    if (sscanf(date, "%3s %d %d", month, &day, &year) != 3) return 1767225600; /* 2026-01-01 */
    const char *found = strstr(months, month);
    struct tm tm = {.tm_year = year - 1900, .tm_mon = found ? (int)(found - months) / 3 : 0, .tm_mday = day};
    return mktime(&tm);
}

static void stamp(char out[15], time_t when)
{
    struct tm utc;
    gmtime_r(&when, &utc);
    strftime(out, 15, "%Y%m%d%H%M%S", &utc);
}

static bool generate(void)
{
    char from[15], to[15];
    bool dated = panel_tls_time_known();
    time_t now = dated ? time(NULL) : build_time();
    stamp(from, now - 24 * 3600);
    stamp(to, now + (time_t)PANEL_TLS_DAYS * 24 * 3600);
    ESP_LOGW(TAG, "PANEL_TLS: generating a new device certificate (%s), this takes a few seconds",
             dated ? "dated from NTP time" : "no clock yet: dated from the firmware build");
    bool ok = false;
    mbedtls_pk_context key;
    mbedtls_x509write_cert crt;
    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crt);
    char *cert = calloc(1, 1200), *priv = calloc(1, 400);
    unsigned char serial[16];
    esp_fill_random(serial, sizeof(serial));
    serial[0] = (serial[0] & 0x7f) | 0x01; /* positive, no leading zero byte */
    int r = cert && priv ? 0 : -1;
    if (!r) r = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    if (!r) r = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key), rng, NULL);
    char host[48], subject[80];
    app_get_hostname(host);
    strlcat(host, ".local", sizeof(host));
    snprintf(subject, sizeof(subject), "CN=%s,O=PS5 Wake", host);
    if (!r) {
        mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
        mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
        mbedtls_x509write_crt_set_subject_key(&crt, &key);
        mbedtls_x509write_crt_set_issuer_key(&crt, &key);
        r = mbedtls_x509write_crt_set_subject_name(&crt, subject);
    }
    if (!r) r = mbedtls_x509write_crt_set_issuer_name(&crt, subject);
    if (!r) r = mbedtls_x509write_crt_set_serial_raw(&crt, serial, sizeof(serial));
    if (!r) r = mbedtls_x509write_crt_set_validity(&crt, from, to);
    if (!r) r = mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1);
    if (!r) r = mbedtls_x509write_crt_set_key_usage(&crt, MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
#if MBEDTLS_VERSION_NUMBER >= 0x03050000
    if (!r) {
        static unsigned char ap_ip[4] = {192, 168, 4, 1};
        mbedtls_x509_san_list ip = {0}, dns = {0};
        ip.node.type = MBEDTLS_X509_SAN_IP_ADDRESS;
        ip.node.san.unstructured_name.p = ap_ip;
        ip.node.san.unstructured_name.len = sizeof(ap_ip);
        dns.node.type = MBEDTLS_X509_SAN_DNS_NAME;
        dns.node.san.unstructured_name.p = (unsigned char *)host;
        dns.node.san.unstructured_name.len = strlen(host);
        dns.next = &ip;
        r = mbedtls_x509write_crt_set_subject_alternative_name(&crt, &dns);
    }
#endif
    if (!r) r = mbedtls_x509write_crt_pem(&crt, (unsigned char *)cert, 1200, rng, NULL);
    if (!r) r = mbedtls_pk_write_key_pem(&key, (unsigned char *)priv, 400);
    if (!r) {
        nvs_handle_t handle;
        r = nvs_open("ps5_wol", NVS_READWRITE, &handle);
        if (!r) {
            r = nvs_set_str(handle, "tls_crt", cert);
            if (!r) r = nvs_set_str(handle, "tls_key", priv);
            if (!r) r = nvs_set_u8(handle, "tls_nodate", dated ? 0 : 1);
            if (!r) r = nvs_set_str(handle, "tls_host", host);
            if (!r) r = nvs_commit(handle);
            nvs_close(handle);
        }
    }
    if (r) ESP_LOGE(TAG, "PANEL_TLS: certificate generation failed (-0x%04x)", (unsigned)-r);
    else ok = true;
    if (priv) memset(priv, 0, 400);
    free(cert);
    free(priv);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&key);
    if (ok) unload(); /* next load() reads the new pair */
    return ok;
}

/* Expiry of the stored certificate as time_t (0 if none). *fallback: the long-lived certificate without a clock. */
static time_t stored_expiry(bool *fallback)
{
    *fallback = false;
    if (!load()) return 0;
    mbedtls_x509_crt crt;
    mbedtls_x509_crt_init(&crt);
    time_t expiry = 0;
    if (mbedtls_x509_crt_parse(&crt, (const unsigned char *)cert_pem, strlen(cert_pem) + 1) == 0) {
        struct tm to = {.tm_year = crt.valid_to.year - 1900, .tm_mon = crt.valid_to.mon - 1,
                        .tm_mday = crt.valid_to.day, .tm_hour = crt.valid_to.hour,
                        .tm_min = crt.valid_to.min, .tm_sec = crt.valid_to.sec};
        expiry = mktime(&to); /* the device runs in UTC */
    }
    mbedtls_x509_crt_free(&crt);
    nvs_handle_t handle;
    uint8_t nodate = 0;
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_u8(handle, "tls_nodate", &nodate);
        nvs_close(handle);
    }
    *fallback = nodate != 0;
    return expiry;
}

/* The certificate names the device's .local address: a new device name needs a new certificate. */
static bool host_changed(void)
{
    char want[48], stored[48] = "";
    app_get_hostname(want);
    strlcat(want, ".local", sizeof(want));
    nvs_handle_t handle;
    size_t size = sizeof(stored);
    if (nvs_open("ps5_wol", NVS_READONLY, &handle) == ESP_OK) {
        if (nvs_get_str(handle, "tls_host", stored, &size) != ESP_OK) stored[0] = 0;
        nvs_close(handle);
    }
    return strcmp(want, stored) != 0;
}

bool panel_tls_due(void)
{
    bool fallback = false;
    time_t expiry = stored_expiry(&fallback);
    if (!expiry || host_changed()) return true;
    if (!panel_tls_time_known()) return false;
    if (fallback) return true; /* the clock is known now: replace the build-dated certificate */
    return time(NULL) > expiry - (time_t)PANEL_TLS_RENEW_DAYS * 24 * 3600;
}

bool panel_tls_ensure(bool force, bool *changed)
{
    *changed = false;
    if (force || panel_tls_due()) {
        if (!generate()) return load();
        *changed = true;
    }
    return load();
}

bool panel_tls_get(const char **cert, size_t *cert_len, const char **key, size_t *key_len)
{
    if (!load()) return false;
    *cert = cert_pem;
    *cert_len = strlen(cert_pem) + 1; /* esp_https_server expects the length including the terminator */
    *key = key_pem;
    *key_len = strlen(key_pem) + 1;
    return true;
}

const char *panel_tls_cert_pem(void)
{
    return load() ? cert_pem : NULL;
}

bool panel_tls_info(char fingerprint[96], char expires[11])
{
    fingerprint[0] = expires[0] = 0;
    if (!load()) return false;
    mbedtls_x509_crt crt;
    mbedtls_x509_crt_init(&crt);
    bool ok = mbedtls_x509_crt_parse(&crt, (const unsigned char *)cert_pem, strlen(cert_pem) + 1) == 0;
    if (ok) {
        unsigned char hash[32];
        mbedtls_sha256(crt.raw.p, crt.raw.len, hash, 0);
        for (int i = 0; i < 32; ++i) snprintf(fingerprint + i * 3, 4, i < 31 ? "%02X:" : "%02X", hash[i]);
        snprintf(expires, 11, "%04d-%02d-%02d", crt.valid_to.year, crt.valid_to.mon, crt.valid_to.day);
    }
    mbedtls_x509_crt_free(&crt);
    return ok;
}
