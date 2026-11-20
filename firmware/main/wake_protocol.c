#include "wake_protocol.h"
#include <string.h>

void wake_build_identity(uint8_t packet[16], const uint8_t controller[6])
{
    const uint8_t header[9] = {1, 0x31, 0xfc, 12, 'B', 'R', 'C', '6', 8};
    memset(packet, 0, 16);
    memcpy(packet, header, sizeof(header));
    for (unsigned i = 0; i < 6; ++i) packet[9 + i] = controller[5 - i];
    /* packet[15] = 0: preserve the exact address, disable Soft-BR address derivation. */
}

void wake_build_page(uint8_t packet[17], const uint8_t ps5[6])
{
    memset(packet, 0, 17);
    packet[0] = 1; packet[1] = 5; packet[2] = 4; packet[3] = 13;
    for (unsigned i = 0; i < 6; ++i) packet[4 + i] = ps5[5 - i];
    packet[10] = 0x18; packet[11] = 0xcc; packet[12] = 2; packet[16] = 1;
}
