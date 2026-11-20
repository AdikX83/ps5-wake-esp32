#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wol.h"

int main(void)
{
    const uint8_t mac[6] = {0xdc, 0x1e, 0xd5, 0x11, 0x22, 0x33};
    uint8_t output[6];
    assert(parse_mac("DC:1e:D5:11:22:33", output) && !memcmp(mac, output, 6));
    /* Bluetooth BD_ADDR does not use the Ethernet multicast bit convention. */
    assert(parse_mac("AB:12:34:56:78:90", output));
    const char *bad[] = {"", "00:00:00:00:00:00", "FF:FF:FF:FF:FF:FF", "D:1E:D5:11:22:33",
        "DC-1E-D5-11-22-33", "DC:1E:D5:11:22:3G", "DC:1E:D5:11:22:33!", NULL};
    for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) assert(!parse_mac(bad[i], output));
    uint8_t packet[140] = {0};
    memset(packet + 3, 255, 6);
    for (unsigned i = 0; i < 16; ++i) memcpy(packet + 9 + i * 6, mac, 6);
    assert(wol_matches(packet, sizeof(packet), mac));
    assert(wol_matches(packet + 3, 102, mac));
    for (size_t n = 0; n < 102; ++n) assert(!wol_matches(packet + 3, n, mac));
    for (size_t n = 3; n < 105; ++n) {
        packet[n] ^= 1;
        assert(!wol_matches(packet, sizeof(packet), mac));
        packet[n] ^= 1;
    }
    uint8_t other[6] = {1, 2, 3, 4, 5, 6};
    assert(!wol_matches(packet, sizeof(packet), other));
    assert(!wol_matches(NULL, 102, mac));
    assert(!wol_matches(packet, sizeof(packet), NULL));
    puts("WoL parser: valid packets, offsets, every truncation, every byte mutation and MAC validation PASS");
    return 0;
}
