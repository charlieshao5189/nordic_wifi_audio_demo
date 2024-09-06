#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_audio_player)

FILE(GLOB net_sources src/net/*.c)
FILE(GLOB audio_sources 
        src/audio/*.c
        src/audio/opus-1.2.1/*.c
        src/audio/libc_opus/*.c
        )

message("showwwwwww audio_sources:")
message("${audio_sources}")

if (CONFIG_NRF700X_AP_MODE)
        list(FILTER net_sources EXCLUDE REGEX ".*wifi_station_mode\\.c$")
else()
        list(FILTER net_sources EXCLUDE REGEX ".*wifi_softap_mode\\.c$")
endif()

target_sources(app PRIVATE
        src/main.c
        ${net_sources}
        ${audio_sources}
        )

target_include_directories(app PRIVATE 
        src
        src/net
        src/audio
        src/audio/opus-1.2.1
        src/audio/libc_opus
        )

target_include_directories(app PUBLIC ${ZEPHYR_BASE}/subsys/net/ip)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
 -DARM_MATH_CM4 \
 -DDISABLE_FLOAT_API \
 -DFIXED_POINT \
 -DUSE_ALLOCA \
 -DHAVE_LRINT \
 -DHAVE_LRINTF \
 -DOPUS_ARM_ASM \
 -DOPUS_ARM_INLINE_EDSP \
 -DOPUS_ARM_INLINE_MEDIA \
 -DOPUS_ARM_MAY_HAVE_EDSP \
 -DOPUS_ARM_PRESUME_EDSP \
 -DCUSTOM_SUPPORT \
 -DVAR_ARRAYS \
 -DOPUS_BUILD \
 -DHAVE_CONFIG_H \
 -g \
")
