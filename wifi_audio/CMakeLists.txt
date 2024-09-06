#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_audio_player)

FILE(GLOB app_sources src/*.c)


message("show app_sources:")
message("${app_sources}")

if (CONFIG_NRF700X_AP_MODE)
        list(FILTER app_sources EXCLUDE REGEX ".*wifi_station_mode\\.c$")
else()
        list(FILTER app_sources EXCLUDE REGEX ".*wifi_softap_mode\\.c$")
endif()

target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE src)
target_include_directories(app PUBLIC ${ZEPHYR_BASE}/subsys/net/ip)

