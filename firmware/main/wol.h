#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool wol_matches(const uint8_t *data, size_t length, const uint8_t target[6]);
bool parse_mac(const char *text, uint8_t result[6]);
