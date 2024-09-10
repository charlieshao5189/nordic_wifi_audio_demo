#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_WIFI_STATION_MODE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/wifi_station_mode.c)

target_sources_ifdef(CONFIG_WIFI_SOFTAP_MODE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/wifi_softap_mode.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})