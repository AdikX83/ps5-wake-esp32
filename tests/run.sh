#!/usr/bin/env bash
# Host tests (Linux/macOS/WSL, gcc or clang): the real WoL parser and the wake HCI packets against the pinned
# Soft-BR engine, with AddressSanitizer and UndefinedBehaviorSanitizer.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
out="${TMPDIR:-/tmp}/ps5-wake-tests"
mkdir -p "$out"
cc="${CC:-gcc}"
flags=(-std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all)
main="$root/firmware/main"
br="$root/third_party/esp32-p4-c6-le-audio/br_ll"
"$cc" "${flags[@]}" -I"$main" "$root/tests/test_wol.c" "$main/wol.c" -o "$out/test_wol"
"$out/test_wol"
"$cc" "${flags[@]}" -I"$main" -I"$br" "$root/tests/test_wake_protocol.c" "$main/wake_protocol.c" \
    "$br/br_proto.c" "$br/br_hop.c" "$br/br_air.c" "$br/br_crypto.c" "$br/br_engine.c" -o "$out/test_wake_protocol"
"$out/test_wake_protocol"
echo "All host tests passed"
