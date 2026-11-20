#pragma once
#include <stdint.h>
void wake_build_identity(uint8_t packet[16], const uint8_t controller[6]);
void wake_build_page(uint8_t packet[17], const uint8_t ps5[6]);
