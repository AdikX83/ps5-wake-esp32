# Soft-BR (MIT, github.com/ernimen7/esp32-p4-c6-le-audio) is a git submodule pinned to a reviewed commit.
get_filename_component(SOFTBR "${CMAKE_CURRENT_LIST_DIR}/../../third_party/esp32-p4-c6-le-audio" ABSOLUTE)
get_filename_component(READER "${CMAKE_CURRENT_LIST_DIR}/../../tools/reader" ABSOLUTE)
execute_process(COMMAND git -C "${SOFTBR}" rev-parse HEAD OUTPUT_VARIABLE SOFTBR_REV
                OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE GIT_RESULT)
if(NOT GIT_RESULT EQUAL 0 OR NOT SOFTBR_REV STREQUAL "0fafdd428ea26f8bb700d67d72e2dcf12241f4c5")
    message(FATAL_ERROR "Expected the pinned Soft-BR submodule 0fafdd4 in third_party/esp32-p4-c6-le-audio "
                        "(run: git submodule update --init)")
endif()
set(BR "${SOFTBR}/br_ll")
set(ADAPTER "${SOFTBR}/c6_firmware/main")
# br_c6_radio.c is built from softbr_patched/ (pinned upstream + br_c6_radio.patch); tools/verify_build.py checks
# both hashes. project_include.cmake applies the single Soft-BR scheduler hook to libble_app.a.
idf_component_register(
    SRCS "main.c" "wake_radio.c" "wake_protocol.c" "wol.c" "portal.c" "panel_tls.c" "ha_mqtt.c" "ps5_state.c" "led.c"
         "${BR}/br_proto.c" "${BR}/br_hop.c" "${BR}/br_air.c" "${BR}/br_crypto.c" "${BR}/br_engine.c"
         "${ADAPTER}/br_c6_core.c" "${CMAKE_CURRENT_LIST_DIR}/../softbr_patched/br_c6_radio.c" "${ADAPTER}/br_c6_crash.c"
         "${SOFTBR}/p4_bt_classic/src/btc_lmp.cpp"
    INCLUDE_DIRS "." "${BR}" "${ADAPTER}" "${SOFTBR}/cis_ll" "${SOFTBR}/p4_bt_classic/src"
    EMBED_TXTFILES "portal.html" "${READER}/ps5-wake-reader.cmd" "${READER}/ps5_wake_reader.py"
    REQUIRES bt nvs_flash esp_http_server esp_https_server mdns mbedtls mqtt esp_wifi esp_netif lwip esp_timer esp_system esp_hw_support
             esp_rom esp_app_format freertos json driver vfs riscv)
# esp_panic_handler: Soft-BR crash guard; esp_netif_receive: raw Ethernet Wake-on-LAN frames (EtherType 0x0842).
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=esp_panic_handler" "-Wl,--wrap=esp_netif_receive")
