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
        #src/audio
        src/net
        src/drivers
        src/modules
        src/utils
        src/utils/macros
        ${ZEPHYR_NRF_MODULE_DIR}/boards/arm/nrf5340_audio_dk_nrf5340
        )

#add_subdirectory(src/audio)
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


# # For Opus building
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
#  -DARM_MATH_CM4 \
#  -DDISABLE_FLOAT_API \
#  -DFIXED_POINT \
#  -DUSE_ALLOCA \
#  -DHAVE_LRINT \
#  -DHAVE_LRINTF \
#  -DOPUS_ARM_ASM \
#  -DOPUS_ARM_INLINE_EDSP \
#  -DOPUS_ARM_INLINE_MEDIA \
#  -DOPUS_ARM_MAY_HAVE_EDSP \
#  -DOPUS_ARM_PRESUME_EDSP \
#  -DCUSTOM_SUPPORT \
#  -DVAR_ARRAYS \
#  -DOPUS_BUILD \
#  -DHAVE_CONFIG_H \
#  -g \
# ")

# Exclude files with main function
list(APPEND EXCLUDE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/celt/dump_modes/dump_modes.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/src_c/opus_compare.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/src_c/opus_demo.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/src_c/repacketizer_demo.c"
)

FILE(GLOB audio_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/*.c

    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/celt/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/celt/arm/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/celt/dump_modes/*.c

    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/silk/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/silk/arm/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/silk/fixed/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/silk/fixed/arm/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/silk/float/*.c

    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/libc_opus/*.c

    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/opus/src_c/*.c
    )

foreach(file ${EXCLUDE_FILES})
    list(REMOVE_ITEM audio_sources ${file})
endforeach()

target_sources(app PRIVATE ${audio_sources})

target_include_directories(app PRIVATE
    src/audio
    src/audio/opus
    src/audio/opus/libc_opus
    src/audio/opus/include
    src/audio/opus/celt
    src/audio/opus/celt/arm
    src/audio/opus/celt/dump_modes
    src/audio/opus/silk
    src/audio/opus/silk/arm
    src/audio/opus/silk/fixed
    src/audio/opus/silk/fixed/arm
    src/audio/opus/silk/float
    ${ZEPHYR_NRF_MODULE_DIR}/boards/arm/nrf5340_audio_dk_nrf5340
)


# For OPUS
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
-DARM_MATH_CM4 \
-DUSE_ALLOCA \
-DHAVE_LRINT \
-DHAVE_LRINTF \
-DCUSTOM_SUPPORT \
-DVAR_ARRAYS \
-DOPUS_BUILD \
-DHAVE_CONFIG_H \
-g \
-mfpu=fpv5-sp-d16 \
-mfloat-abi=hard \
-mcpu=cortex-m33 \
-mthumb \
")