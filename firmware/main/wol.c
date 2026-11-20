#include "wol.h"
#include <string.h>

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool parse_mac(const char *text, uint8_t result[6])
{
    if (!text || !result || strlen(text) != 17) return false;
    uint8_t temporary[6];
    bool nonzero = false, nonbroadcast = false;
    for (unsigned i = 0; i < 6; ++i) {
        int hi = hex_digit(text[i * 3]), lo = hex_digit(text[i * 3 + 1]);
        if (hi < 0 || lo < 0 || (i < 5 && text[i * 3 + 2] != ':')) return false;
        temporary[i] = (uint8_t)((hi << 4) | lo);
        nonzero |= temporary[i] != 0;
        nonbroadcast |= temporary[i] != 255;
    }
    if (!nonzero || !nonbroadcast) return false;
    memcpy(result, temporary, 6);
    return true;
}

bool wol_matches(const uint8_t *data, size_t length, const uint8_t target[6])
{
    if (!data || !target || length < 102) return false;
    for (size_t start = 0; start <= length - 102; ++start) {
        unsigned i;
        for (i = 0; i < 6 && data[start + i] == 255; ++i) {}
        if (i != 6) continue;
        for (i = 0; i < 16; ++i) {
            if (memcmp(data + start + 6 + i * 6, target, 6)) break;
        }
        if (i == 16) return true;
    }
    return false;
}
