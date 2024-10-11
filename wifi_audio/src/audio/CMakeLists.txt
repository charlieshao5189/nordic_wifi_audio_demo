#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Os -march=armv8-m.main+dsp+fp -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -Wa,-mimplicit-it=thumb -g")
option(HAVE_LRINT "Enable lrint() support" ON)
option(HAVE_LRINTF "Enable lrintf() support" ON)
option(OPUS_VAR_ARRAYS "use variable length arrays for stack arrays." ON)
option(OPUS_FIXED_POINT "Compile as fixed-point (for machines without a fast enough FPU)." ON)
option(OPUS_FIXED_POINT_DEBUG "debug fixed-point implementation." OFF)
option(OPUS_ENABLE_FLOAT_API "compile with the floating point API (for machines with float library)." OFF)
option(OPUS_DISABLE_INTRINSICS "disable all intrinsics optimizations." ON)
option(OPUS_HARDENING "run-time checks that are cheap and safe for use in production." OFF)
option(OPUS_INSTALL_PKG_CONFIG_MODULE "install pkg-config module." OFF)

add_subdirectory(opus)

get_target_property(OPUS_COMPILE_OPTIONS opus COMPILE_OPTIONS)
message(STATUS "Compile options for target 'opus': ${OPUS_COMPILE_OPTIONS}")

target_link_libraries(app PRIVATE opus)

FILE(GLOB audio_sources 
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        # ${CMAKE_CURRENT_SOURCE_DIR}/opus/*.c
        )

target_sources(app PRIVATE
        ${audio_sources}
        # opus/arm/celt_pitch_xcorr_arm_gcc.s
        )

target_include_directories(app PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        # ${CMAKE_CURRENT_SOURCE_DIR}/opus
        # ${CMAKE_CURRENT_SOURCE_DIR}/opus/arm
        # ${CMAKE_CURRENT_SOURCE_DIR}/libc_opus
        )


