#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

FILE(GLOB audio_sources 
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/opus-1.2.1/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libc_opus/*.c
        )


target_sources(app PRIVATE
        ${audio_sources}
        )

target_include_directories(app PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/opus-1.2.1
        ${CMAKE_CURRENT_SOURCE_DIR}/libc_opus
        )
