#pragma once
#include <stdbool.h>
#include <stddef.h>

/* Device certificate for the HTTPS home-network panel (see panel_tls.c). */

/* Makes sure a usable certificate exists: generates one if missing, if force is set, if it expires within 30 days,
 * or if it is the no-clock fallback and the time is now known. *changed tells whether a new one was made. */
bool panel_tls_ensure(bool force, bool *changed);
bool panel_tls_due(void); /* a new certificate is needed (missing, expiring, or fallback with a known clock) */
/* Current PEM pair; buffers stay valid until the next panel_tls_ensure() that changes the certificate. */
bool panel_tls_get(const char **cert, size_t *cert_len, const char **key, size_t *key_len);
const char *panel_tls_cert_pem(void);                    /* NULL if there is no certificate */
bool panel_tls_info(char fingerprint[96], char expires[11]); /* SHA-256 "AA:BB:…", expiry "YYYY-MM-DD" */
bool panel_tls_time_known(void);                          /* the clock was set by NTP */
