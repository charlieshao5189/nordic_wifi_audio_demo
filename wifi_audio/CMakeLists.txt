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

string(TIMESTAMP NRF5340_AUDIO_CORE_APP_COMP_DATE "%a %b %d %H:%M:%S %Y")
message(NRF5340_WIFI_AUDIO_COMP_DATE="${NRF5340_AUDIO_CORE_APP_COMP_DATE}")

# Generate fw_info_app.c
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/utils/fw_info_app.c.in"
               "${CMAKE_BINARY_DIR}/fw_info_app.c"
               @ONLY)

# Target sources below are specific to the nRF5340 Audio DK HW
target_sources(app PRIVATE
    ${CMAKE_BINARY_DIR}/fw_info_app.c
    )
if (CONFIG_AUDIO_HEADSET)
	add_subdirectory(wifi_audio_headset)
endif()

if (CONFIG_AUDIO_GATEWAY)
	add_subdirectory(wifi_audio_gateway)
endif()
# target_include_directories(app PUBLIC
zephyr_library_include_directories(
        include
        src
        src/audio
        src/net
        src/drivers
        src/modules
        src/utils
        src/utils/macros
        # ${ZEPHYR_NRF_MODULE_DIR}/boards/arm/nrf5340_audio_dk_nrf5340
        )

zephyr_library_include_directories(app PRIVATE
${ZEPHYR_NRF_MODULE_DIR}/boards/arm/nrf5340_audio_dk_nrf5340)

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

# build counter
# Set a cached variable for the build count (default is 0)
if(NOT DEFINED WIFI_AUDIO_CMAKE_BUILD_COUNT)
    set(WIFI_AUDIO_CMAKE_BUILD_COUNT 0 CACHE STRING "Number of times the project has been built")
endif()
# Increment the build count
math(EXPR WIFI_AUDIO_CMAKE_BUILD_COUNT "${WIFI_AUDIO_CMAKE_BUILD_COUNT} + 1")
# Update the cached value
set(WIFI_AUDIO_CMAKE_BUILD_COUNT ${WIFI_AUDIO_CMAKE_BUILD_COUNT} CACHE STRING "Number of times the project has been built" FORCE)
# Print the current build count to the console
message(STATUS "Build count: ${WIFI_AUDIO_CMAKE_BUILD_COUNT}")