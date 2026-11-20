#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "br_engine.h"
#include "wake_protocol.h"

static uint8_t event[260];
static void on_hci(void *ctx, const uint8_t *packet, uint16_t length)
{ (void)ctx; assert(length <= sizeof(event)); memcpy(event, packet, length); }
static int on_pair(void *ctx, const br_radio_tx_req_t *tx, const br_radio_rx_req_t *rx)
{ (void)ctx; (void)tx; (void)rx; return 0; }
static uint32_t now(void *ctx) { (void)ctx; return 100000u; }

int main(void)
{
    br_platform_ops_t ops = {.hci_to_host = on_hci, .schedule_tx_rx = on_pair, .now_us = now};
    void *storage = calloc(1, br_engine_storage_size());
    assert(storage);
    br_engine_t *engine = br_engine_init(storage, br_engine_storage_size(), &ops, NULL);
    assert(engine);
    /* Asymmetric test addresses make accidentally swapped byte order visible. */
    const uint8_t controller[6] = {0x9b, 0x12, 0x34, 0x56, 0x78, 0xa9};
    const uint8_t ps5[6] = {0x4c, 0x21, 0x43, 0x65, 0x87, 0xba};
    uint8_t identity[16], page[17], readback[6];
    wake_build_identity(identity, controller);
    assert(br_engine_hci_from_host(engine, identity, sizeof(identity)));
    assert(event[1] == 0x0e && event[4] == 0x31 && event[5] == 0xfc && event[6] == 0);
    br_engine_get_bdaddr(engine, readback);
    for (unsigned i = 0; i < 6; ++i) assert(readback[i] == controller[5 - i]);
    wake_build_page(page, ps5);
    assert(br_engine_hci_from_host(engine, page, sizeof(page)));
    assert(event[1] == 0x0f && event[3] == 0 && event[5] == 5 && event[6] == 4);
    assert(br_engine_get_state(engine) == BR_STATE_PAGE);
    const uint8_t stop[] = {1, 0x31, 0xfc, 5, 'B', 'R', 'C', '6', 9};
    assert(br_engine_hci_from_host(engine, stop, sizeof(stop)));
    assert(br_engine_get_state(engine) == BR_STATE_IDLE);
    free(storage);
    puts("Wake protocol against real Soft-BR engine: exact paired identity, page command and stop PASS");
    return 0;
}
