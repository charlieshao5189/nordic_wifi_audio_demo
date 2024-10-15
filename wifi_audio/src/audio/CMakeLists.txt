#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_SW_CODEC_OPUS) # Opus codec is enabled, include opus sources in the build
        target_compile_definitions(app PUBLIC HAVE_CONFIG_H)
        target_compile_definitions(app PUBLIC ARM_MATH_CM4)
        target_compile_definitions(app PUBLIC CUSTOM_SUPPORT)
        target_compile_definitions(app PUBLIC HAVE_LRINT)
        target_compile_definitions(app PUBLIC HAVE_LRINT)

        FILE(GLOB opus_codec
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/analysis.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/extensions.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/mlp.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/mlp_data.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/opus.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/opus_decoder.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/opus_encoder.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/opus_multistream.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/opus_multistream_decoder.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/opus_multistream_encoder.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src/repacketizer.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/celt/*.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/celt/arm/arm_celt_map.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/celt/arm/armcpu.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/silk/*.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/silk/fixed/*.c
                ${CMAKE_CURRENT_SOURCE_DIR}/opus_interface/*.c
                )
        list(REMOVE_ITEM opus_codec "${CMAKE_CURRENT_SOURCE_DIR}/opus/celt/opus_custom_demo.c") # Do not include custom demo code in the build

        target_sources(app PRIVATE
                ${opus_codec}
                )

        target_include_directories(app PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/opus
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/src
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/include
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/celt
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/celt/arm
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/silk
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/silk/arm
                ${CMAKE_CURRENT_SOURCE_DIR}/opus/silk/fixed
                ${CMAKE_CURRENT_SOURCE_DIR}/opus_interface
                )
endif()

FILE(GLOB audio_sources 
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        )

target_sources(app PRIVATE
        ${audio_sources}
        )

target_include_directories(app PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        )


