# PS5 Wake for ESP32-C6 - ESP-IDF project.
# Build: idf.py set-target esp32c6 build   (ESP-IDF 5.5.5 exactly, see docs/technical/building.md)
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include($ENV{IDF_PATH}/tools/cmake/version.cmake)
if(NOT IDF_VERSION_MAJOR EQUAL 5 OR NOT IDF_VERSION_MINOR EQUAL 5 OR NOT IDF_VERSION_PATCH EQUAL 5)
    message(FATAL_ERROR "The Soft-BR radio adapter requires ESP-IDF 5.5.5 exactly")
endif()
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../VERSION" PS5_WAKE_VERSION LIMIT_COUNT 1)
# Re-run the configuration when VERSION changes, so an incremental build never keeps an old version string.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../VERSION")
set(PROJECT_VER "${PS5_WAKE_VERSION}")
set(COMPONENTS main)
project(ps5_wake)
