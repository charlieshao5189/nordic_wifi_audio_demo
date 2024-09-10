#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Flag which defines whether application is compiled as gateway/dongle or headset
add_compile_definitions(HEADSET=1)
add_compile_definitions(GATEWAY=2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(wifi_audio_transceiver)

string(TIMESTAMP NRF5340_WIFI_AUDIO_COMP_DATE "%a %b %d %H:%M:%S %Y")

# Generate fw_info_app.c
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/utils/fw_info_app.c.in"
               "${CMAKE_BINARY_DIR}/fw_info_app.c"
               @ONLY)

# Target sources below are specific to the nRF5340 Audio DK HW
target_sources(app PRIVATE
    ${CMAKE_BINARY_DIR}/fw_info_app.c
    )

target_sources(app PRIVATE
        src/main.c
        )
target_include_directories(app PRIVATE 
        src
        src/audio
        src/net
        src/drivers
        src/modules
        src/utils
        src/utils/macros
        ${ZEPHYR_NRF_MODULE_DIR}/boards/arm/nrf5340_audio_dk_nrf5340
        )

add_subdirectory(src/audio)
add_subdirectory(src/net)
add_subdirectory(src/drivers)
add_subdirectory(src/modules)
add_subdirectory(src/utils)

## Cirrus Logic
if (CONFIG_HW_CODEC_CIRRUS_LOGIC)
    if (ZEPHYR_CIRRUS_LOGIC_MODULE_DIR)
        add_subdirectory(${ZEPHYR_CIRRUS_LOGIC_MODULE_DIR} cirrus_logic_bin_dir)
    else()
        message(FATAL_ERROR "Cirrus Logic/sdk-mcu-drivers repository not found\n")
    endif()
endif()


# For Opus building
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